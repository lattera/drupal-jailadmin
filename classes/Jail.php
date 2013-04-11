<?php

class Jail {
    public $name;
    public $path;
    public $dataset;
    public $routes;
    public $network;
    public $services;
    public $mounts;
    public $autoboot;
    public $hostname;
    public $BEs;
    private $_snapshots;

    function __construct() {
        $this->network = array();
        $this->services = array();
        $this->_snapshots = array();
        $this->BEs = array();
    }

    public static function LoadAll() {
        $result = db_query('SELECT name FROM {jailadmin_jails}');
        $jails = array();

        foreach ($result as $record)
            $jails[] = Jail::Load($record->name);

        return $jails;
    }

    public static function Load($name) {
        $result = db_query('SELECT * FROM {jailadmin_jails} WHERE name = :name', array(':name' => $name));

        $record = $result->fetchAssoc();
        return Jail::LoadFromRecord($record);
    }

    protected static function LoadFromRecord($record=array()) {
        if (count($record) == 0)
            return FALSE;

        $jail = new Jail;
        $jail->name = $record['name'];
        $jail->dataset = $record['dataset'];
        $jail->autoboot = ($record['autoboot'] == 1);
        $jail->hostname = $record['hostname'];
        $jail->network = NetworkDevice::Load($jail);
        $jail->services = Service::Load($jail);
        $jail->mounts = Mount::Load($jail);

        $result = db_select('jailadmin_routes', 'jr')
            ->fields('jr', array('source', 'destination'))
            ->condition('jail', $jail->name)
            ->execute();

        $jail->routes = array();
        foreach ($result as $route) {
            $arr = array();
            $arr['source'] = $route->source;
            $arr['destination'] = $route->destination;

            $jail->routes[] = $arr;
        }

        $jail->load_boot_environments();

        if (count($jail->BEs)) {
            $jail->path = $jail->GetActiveBE()["mountpoint"];
        } else {
            $jail->path = trim(exec("/sbin/zfs get -H -o value mountpoint {$jail->dataset}"));
        }

        return $jail;
    }

    protected function load_boot_environments() {
        $this->BEs = array();

        $dspec = array(
            0 => array("pipe", "r"),
            1 => array("pipe", "w"),
            2 => array("pipe", "w")
        );

        $proc = proc_open("/sbin/zfs list -H -oname -r -t filesystem {$this->dataset}", $dspec, $pipes);
        if (!is_resource($proc))
            return FALSE;

        $datasets = stream_get_contents($pipes[1]);

        proc_close($proc);

        $datasets = explode("\n", trim($datasets));

        /* If we have less than three datasets, we're not using BEs */
        if (count($datasets) < 3)
            return;

        $i=0;
        foreach ($datasets as $dataset) {
            if ($i < 2) {
                /* Ignore the first two entries. They're the parent datasets */
                $i++;
                continue;
            }

            $active = false;

            $prop = trim(exec("/sbin/zfs get -H -o value jailadmin:be_active {$dataset}"));
            if ($prop == "true")
                $active = true;

            $mountpoint=trim(exec("/sbin/zfs get -H -o value mountpoint {$dataset}"));
            $pretty = substr($dataset, strrpos($dataset, "/")+1);

            $this->BEs[] = array(
                "dataset" => $dataset,
                "mountpoint" => $mountpoint,
                "pretty_dataset" => $pretty,
                "active" => $active,
            );
        }
    }

    private function load_snapshots() {
        if (count($this->BEs)) {
            foreach ($this->BEs as $be) {
                $snapshots = array();
                exec("/sbin/zfs list -rH -oname -t snapshot {$be["dataset"]}", $snapshots);
                foreach ($snapshots as $snapshot) {
                    $snapshot = trim($snapshot);
                    $this->_snapshots[] = substr($snapshot, strrpos($snapshot, "/")+1);
                }
            }
        } else {
            $snapshots = array();
            exec("/sbin/zfs list -rH -oname -t snapshot {$this->dataset}", $snapshots);
            foreach ($snapshots as $snapshot) {
                $snapshot = trim($snapshot);
                $this->_snapshots[] = substr($snapshot, strrpos($snapshot, "@")+1);
            }
        }
    }

    public function GetSnapshots() {
        if (count($this->_snapshots) > 0)
            return $this->_snapshots;

        $this->load_snapshots();

        return $this->_snapshots;
    }

    public function RevertSnapshot($snapshot) {
        $snap = $this->ResolveSnapshot($snapshot);
        exec("/usr/local/bin/sudo /sbin/zfs rollback -rf \"{$snap}\"");

        return TRUE;
    }

    public function ResolveSnapshot($snapshot) {
        $bename = explode("@", $snapshot);
        if (count($this->BEs)) {
            foreach ($this->BEs as $be) {
                if ($be["pretty_dataset"] == $bename[0]) {
                    return $be["dataset"] . "@" . $bename[1];
                }
            }
        } else {
            return $this->dataset . "@" . $snapshot;
        }
    }

    public function CreateTemplateFromSnapshot($snapshot, $name='') {
        $snap = $this->ResolveSnapshot($snapshot);

        if ($name == '')
            $name = $snap;

        db_insert('jailadmin_templates')
            ->fields(array(
                'name' => $name,
                'snapshot' => $snap,
            ))
            ->execute();

        return TRUE;
    }

    public function DeleteSnapshot($snapshot) {
        $snap = $this->ResolveSnapshot($snapshot);
        exec("/usr/local/bin/sudo /sbin/zfs destroy -rf \"{$snap}\"");

        return true;
    }

    public function IsOnline() {
        $o = exec("/usr/sbin/jls -n -j \"{$this->name}\" jid 2>&1 | grep -v \"{$this->name}\"");
        return strlen($o) > 0;
    }

    public function IsOnlineString() {
        if ($this->IsOnline())
            return 'Online';

        return 'Offline';
    }

    public function NetworkStatus() {
        $status = "";

        foreach ($this->network as $n) {
            $status .= (strlen($status) ? ", " : "") . "{$n->bridge->name}[{$n->device} { ";
            $i= 0;
            if ($n->is_span) {
                $i++;
                $status .= "(SPAN)";
            }

            if ($n->dhcp) {
                $o = "";
                if ($this->IsOnline())
                    $o = exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" ifconfig {$n->device}b 2>&1 | grep -w inet | awk '{print $2;}'");
                $status .= ($i++ > 0 ? "," : "") . " (DHCP" . (strlen($o) ? ": {$o}" : "") . ")";
            }

            if (!count($n->ips))
                $status .= ($i++ > 0 ? "," : " ") . " (NO STATIC IP)";

            foreach ($n->ips as $ip) {
                $status .= ($i++ > 0 ? "," : "") . " {$ip}";
            }

            $status .= ($n->IsOnline()) ? " (online)" : " (offline)";

            $status .= " }]";
        }

        return $status;
    }

    public function Start() {
        if ($this->IsOnline())
            if ($this->Stop() == FALSE)
                return FALSE;

        $hostname = (strlen($this->hostname) == 0) ? $this->name : $this->hostname;

        exec("/usr/local/bin/sudo /sbin/mount -t devfs devfs {$this->path}/dev");
        exec("/usr/local/bin/sudo /usr/sbin/jail -c vnet 'name={$this->name}' 'host.hostname={$hostname}' 'path={$this->path}' persist");

        foreach ($this->network as $n)
            $n->BringHostOnline();

        foreach ($this->network as $n)
            $n->BringGuestOnline();

        foreach ($this->routes as $route) {
            $inet = (strstr($route['destination'], ':') === FALSE) ? 'inet' : 'inet6';

            exec("/usr/local/bin/sudo /usr/sbin/jexec '{$this->name}' route add -{$inet} '{$route['source']}' '{$route['destination']}'");
        }

        foreach ($this->mounts as $mount) {
            $command = "/usr/local/bin/sudo /sbin/mount ";
            if (strlen($mount->driver))
                $command .= "-t {$mount->driver} ";
            if (strlen($mount->options))
                $command .= "-o {$mount->options} ";

            if (!is_dir("{$this->path}/{$mount->target}"))
                exec("/usr/local/bin/sudo /bin/mkdir -p '{$this->path}/{$mount->target}'");

            exec("{$command} {$mount->source} {$this->path}/{$mount->target}");
        }

        foreach ($this->services as $service)
            exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" {$service->path} start");

        exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /bin/sh /etc/rc");

        foreach ($this->network as $n)
            if ($n->ipv6)
                exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /sbin/ifconfig {$n->device}b inet6 -ifdisabled");

        exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /sbin/ifconfig lo0 inet 127.0.0.1");

        return TRUE;
    }

    public function Stop() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        exec("/usr/local/bin/sudo /usr/sbin/jail -r \"{$this->name}\"");
        exec("/usr/local/bin/sudo /sbin/umount {$this->path}/dev");

        foreach ($this->mounts as $mount) {
            $command = "/usr/local/bin/sudo /sbin/umount ";

            exec("{$command} -f {$this->path}/{$mount->target}");
        }

        foreach ($this->network as $n)
            $n->BringOffline();

        return TRUE;
    }

    public function GetActiveBE() {
        foreach ($this->BEs as $be)
            if ($be["active"])
                return $be;

        return FALSE;
    }

    public function Snapshot($base = '') {
        $date = strftime("%F_%T");
        $dataset = $this->dataset;

        if (strlen($base)) {
            foreach ($this->BEs as $be) {
                if ($be["pretty_dataset"] == $base) {
                    $dataset = $be["dataset"];
                    break;
                }
            }
        } else {
            if (count($this->BEs)) {
                foreach ($this->BEs as $be) {
                    if ($be["active"]) {
                        $dataset = $be["dataset"];
                        break;
                    }
                }
            }
        }

        exec("/usr/local/bin/sudo /sbin/zfs snapshot {$dataset}@{$date}");

        return "{$dataset}@{$date}";
    }

    public function UpgradeWorld() {
        if ($this->IsOnline())
            return FALSE;

        $date = strftime("%F_%T");

        if ($this->Snapshot() == FALSE)
            return FALSE;

        exec("cd /usr/src; /usr/local/bin/sudo make installworld DESTDIR={$this->path} > \"/tmp/upgrade-{$this->name}-{$date}.log\" 2>&1");

        return TRUE;
    }

    public function CreateNewBE($name, $base='') {
        $snap = $this->Snapshot($base);
        $oldbe = array();

        exec("/usr/local/bin/sudo /sbin/zfs clone -o jailadmin:be_active=false {$snap} {$this->dataset}/ROOT/{$name}");

        return TRUE;
    }

    public function ActivateBE($name) {
        if ($this->IsOnline())
            return FALSE;

        $oldbe = $this->GetActiveBE();

        exec("/usr/local/bin/sudo /sbin/zfs set jailadmin:be_active=false {$oldbe["dataset"]}");
        exec("/usr/local/bin/sudo /sbin/zfs promote {$this->dataset}/ROOT/{$name}");
        exec("/usr/local/bin/sudo /sbin/zfs set jailadmin:be_active=true {$this->dataset}/ROOT/{$name}");

        $this->load_boot_environments();
        $this->path = $this->GetActiveBE()["mountpoint"];

        return TRUE;
    }

    public function DeleteBE($name) {
        foreach ($this->BEs as $be) {
            if ($be["pretty_dataset"] == $name) {
                if ($be["active"])
                    return FALSE;

                exec("/usr/local/bin/sudo /sbin/zfs destroy -r {$be["dataset"]}");
                return TRUE;
            }
        }
    }

    public function SetupServices() {
        if (count($this->network)) {
            $ip = Network::SanitizedIP($this->network[0]->ip);
            exec("/usr/local/bin/sudo /bin/sh -c '/bin/echo \"ListenAddress {$ip}\" >> {$this->path}/etc/ssh/sshd_config'");
            exec("/usr/local/bin/sudo /bin/sh -c '/bin/echo sshd_enable=\\\"YES\\\" >> {$this->path}/etc/rc.conf'");
        }
    }

    public function Create($template='', $usebe=FALSE) {
        $dataset = $this->dataset;
        $opts = "";

        if (strlen($template)) {
            /* If $template is set, we need to create this jail */

            if ($usebe) {
                exec("/usr/local/bin/sudo /sbin/zfs create -omountpoint=none {$this->dataset}");
                exec("/usr/local/bin/sudo /sbin/zfs create -omountpoint=none {$this->dataset}/ROOT");
                $dataset .= "/ROOT/base";

                $opts = "-o jailadmin:be_active=true";
            }

            exec("/usr/local/bin/sudo zfs clone {$opts} {$template} {$dataset}");
        }

        db_insert('jailadmin_jails')
            ->fields(array(
                'name' => $this->name,
                'dataset' => $this->dataset,
                'hostname' => $this->hostname,
            ))->execute();
    }

    public function Delete($destroy) {
        foreach ($this->network as $n)
            $n->Delete();

        foreach ($this->services as $s)
            $s->Delete();

        foreach ($this->mounts as $m)
            $m->Delete();

        db_delete('jailadmin_routes')
            ->condition('jail', $this->name)
            ->execute();

        db_delete('jailadmin_jails')
            ->condition('name', $this->name)
            ->execute();

        if ($destroy)
            exec("/usr/local/bin/sudo /sbin/zfs destroy -r {$this->dataset}");
    }

    public function Persist() {
        db_update('jailadmin_jails')
            ->fields(array(
                'dataset' => $this->dataset,
                'autoboot' => ($this->autoboot ? 1 : 0),
                'hostname' => $this->hostname,
            ))
            ->condition('name', $this->name)
            ->execute();
    }
}
