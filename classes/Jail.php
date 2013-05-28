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
    public $HasBEs;
    public $MultipleActiveBEs;
    public $ZeroActiveBEs;
    private $_snapshots;

    function __construct() {
        $this->network = array();
        $this->services = array();
        $this->_snapshots = array();
        $this->BEs = array();
        $this->MultipleActiveBEs = FALSE;
        $this->ZeroActiveBEs = TRUE;
        $this->path = "";
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

        if ($jail->HasBEs) {
            $be = $jail->GetActiveBE();

            if ($be !== FALSE)
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

        $proc = proc_open("/sbin/zfs list -H -oname -r -t filesystem {$this->dataset}/ROOT", $dspec, $pipes);
        if (!is_resource($proc))
            return FALSE;

        $datasets = stream_get_contents($pipes[1]);

        proc_close($proc);

        $datasets = explode("\n", trim($datasets));

        /* If we have less than three datasets, we're not using BEs */
        if (count($datasets) < 2)
            return;

        $this->HasBEs = true;

        $i=0;
        foreach ($datasets as $dataset) {
            if ($i < 1) {
                /* Ignore the first entry. It's the parent dataset. */
                $i++;
                continue;
            }

            $active = FALSE;

            $prop = trim(exec("/sbin/zfs get -H -o value jailadmin:be_active {$dataset}"));
            if ($prop == "true") {
                if (!($this->ZeroActiveBEs)) {
                    if (!($this->MultipleActiveBEs))
                        drupal_set_message(t('Warning: Jail @jail has multiple active BEs. Please fix manually.', array('@jail' => $this->name)), 'warning');

                    $this->MultipleActiveBEs = TRUE;
                } else {
                    $this->ZeroActiveBEs = FALSE;
                }
                $active = TRUE;
            }

            $mountpoint=trim(exec("/sbin/zfs get -H -o value mountpoint {$dataset}"));
            $pretty = substr($dataset, strrpos($dataset, "/")+1);

            $this->BEs[] = array(
                "dataset" => $dataset,
                "mountpoint" => $mountpoint,
                "pretty_dataset" => $pretty,
                "active" => $active,
            );
        }

        /* Do some sanity checking */
        if ($this->ZeroActiveBEs) {
            drupal_set_message(t('Warning: Jail @jail has zero active BEs. Please fix manually.', array('@jail' => $this->name)), 'warning');
            if ($this->GetActiveBE() !== FALSE)
                $this->ZeroActiveBEs = FALSE;
        } else if ($this->MultipleActiveBEs && $this->GetActiveBE() !== FALSE)
            $this->MultipleActiveBEs = FALSE;
    }

    private function load_snapshots() {
        if ($this->HasBEs) {
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
        if ($this->HasBEs) {
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

    protected function ungraceful_stop($ret=FALSE) {
        exec("/usr/local/bin/sudo /usr/sbin/jail -r \"{$this->name}\"");
        exec("/usr/local/bin/sudo /sbin/umount {$this->path}/dev");

        foreach ($this->mounts as $mount)
            $mount->Unmount();

        foreach ($this->network as $n)
            $n->BringOffline();

        return $ret;
    }

    public function Start() {
        if ($this->MultipleActiveBEs)
            return FALSE;

        if ($this->HasBEs && $this->ZeroActiveBEs)
            return FALSE;

        if (!strlen($this->path))
            return FALSE;

        if ($this->IsOnline())
            if ($this->Stop() == FALSE)
                return FALSE;

        $hostname = (strlen($this->hostname) == 0) ? $this->name : $this->hostname;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/mount -t devfs devfs {$this->path}/dev", $output, $res);
        if ($res != 0)
            return FALSE;

        $output = array();
        exec("/usr/local/bin/sudo /usr/sbin/jail -c vnet 'name={$this->name}' 'host.hostname={$hostname}' 'path={$this->path}' persist", $output, $res);
        if ($res != 0)
            return $this->ungraceful_stop();

        foreach ($this->network as $n)
            if ($n->BringHostOnline() == FALSE)
                return $this->ungraceful_stop();

        foreach ($this->network as $n)
            if ($n->BringGuestOnline() == FALSE)
                return $this->ungraceful_stop();

        foreach ($this->routes as $route) {
            $inet = (strstr($route['destination'], ':') === FALSE) ? 'inet' : 'inet6';

            $output = array();
            exec("/usr/local/bin/sudo /usr/sbin/jexec '{$this->name}' route add -{$inet} '{$route['source']}' '{$route['destination']}'", $output, $res);
            if ($res != 0)
                return $this->ungraceful_stop();
        }

        foreach ($this->mounts as $mount) {
            if ($mount->DoMount() == FALSE) {
                return $this->ungraceful_stop();
            }
        }

        foreach ($this->services as $service) {
            if ($service->Start() != TRUE)
                return $this->ungraceful_stop();
        }

        $output = array();
        exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /bin/sh /etc/rc", $output, $res);
        if ($res != 0)
            return $this->ungraceful_stop();

        foreach ($this->network as $n)
            if ($n->ipv6)
                exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /sbin/ifconfig {$n->device}b inet6 -ifdisabled");

        exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" /sbin/ifconfig lo0 inet 127.0.0.1");

        watchdog("jailadmin", "Jail @jail started", array("@jail" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function Stop() {
        if ($this->MultipleActiveBEs)
            return FALSE;

        if ($this->IsOnline() == FALSE)
            return TRUE;

        $output = array();
        exec("/usr/local/bin/sudo /usr/sbin/jail -r \"{$this->name}\"", $output, $res);
        if ($res != 0)
            return FALSE;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/umount {$this->path}/dev", $output, $res);
        if ($res != 0)
            return FALSE;

        foreach ($this->mounts as $mount) {
            $mount->Unmount();
        }

        foreach ($this->network as $n)
            $n->BringOffline();

        watchdog("jailadmin", "Jail @jail stopped", array("@jail" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function GetActiveBE() {
        if ($this->MultipleActiveBEs || $this->ZeroActiveBEs) {
            /* If the jail is online, then its path has already been set. Make a best guess effort. */
            if (!($this->IsOnline()))
                return FALSE;

            $o = trim(exec("/usr/sbin/jls -j \"{$this->name}\" -n path"));
            if (!strlen($o))
                return FALSE;

            $o = substr($o, strpos($o, "=")+1);
            foreach ($this->BEs as $be) {
                if (trim($be["mountpoint"]) == $o)
                    return $be;
            }

            return FALSE;
        }

        foreach ($this->BEs as $be)
            if ($be["active"])
                return $be;

        return FALSE;
    }

    public function Snapshot($base = '') {
        $date = strftime("%F_%T");
        $dataset = $this->dataset;

        if ($this->HasBEs)
            if ($this->MultipleActiveBEs || $this->ZeroActiveBEs)
                return FALSE;

        if (strlen($base)) {
            foreach ($this->BEs as $be) {
                if ($be["pretty_dataset"] == $base) {
                    $dataset = $be["dataset"];
                    break;
                }
            }
        } else {
            if ($this->HasBEs) {
                foreach ($this->BEs as $be) {
                    if ($be["active"]) {
                        $dataset = $be["dataset"];
                        break;
                    }
                }
            }
        }

        $output = array();
        exec("/usr/local/bin/sudo /sbin/zfs snapshot {$dataset}@{$date}", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Jail @jail snapshotted (@snapshot)", array(
            "@jail" => $this->name,
            "@snapshot" => "{$dataset}@{$date}",
        ), WATCHDOG_INFO);

        return "{$dataset}@{$date}";
    }

    public function UpgradeWorld() {
        if ($this->MultipleActiveBEs)
            return FALSE;

        if ($this->HasBEs && $this->ZeroActiveBEs)
            return FALSE;

        if (!strlen($this->path))
            return FALSE;

        if ($this->IsOnline())
            return FALSE;

        $date = strftime("%F_%T");

        if ($this->Snapshot() == FALSE)
            return FALSE;

        watchdog("jailadmin", "Jail @jail world install started", array("@jail" => $this->name), WATCHDOG_INFO);
        $output = array();
        exec("cd /usr/src; /usr/local/bin/sudo make installworld DESTDIR={$this->path} > \"/tmp/upgrade-{$this->name}-{$date}.log\" 2>&1", $output, $res);
        if ($res != 0) {
            watchdog("jailadmin", "Jail @jail world install failed", array("@jail" => $this->name), WATCHDOG_INFO);
            return FALSE;
        }

        watchdog("jailadmin", "Jail @jail world install finished", array("@jail" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function CreateNewBE($name, $base='') {
        $snap = $this->Snapshot($base);
        $oldbe = array();

        $output = array();
        exec("/usr/local/bin/sudo /sbin/zfs clone -o jailadmin:be_active=false {$snap} {$this->dataset}/ROOT/{$name}", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Jail @jail BE @be created", array(
            "@jail" => $this->name,
            "@be" => $name
        ), WATCHDOG_INFO);

        return TRUE;
    }

    public function ResolveBE($name) {
        foreach ($this->BEs as $be)
            if ($be["pretty_dataset"] == $name)
                return $be;

        return FALSE;
    }

    public function ActivateBE($name) {
        if ($this->IsOnline() || $this->MultipleActiveBEs)
            return FALSE;

        $oldbe = $this->GetActiveBE();

        if ($oldbe["active"]) {
            $output = array();
            exec("/usr/local/bin/sudo /sbin/zfs set jailadmin:be_active=false {$oldbe["dataset"]}", $output, $res);
            if ($res != 0)
                return FALSE;
        }

        $output = array();
        exec("/usr/local/bin/sudo /sbin/zfs promote {$this->dataset}/ROOT/{$name}", $output, $res);
        if ($res != 0)
            return FALSE;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/zfs set jailadmin:be_active=true {$this->dataset}/ROOT/{$name}", $output, $res);
        if ($res != 0)
            return FALSE;

        $this->load_boot_environments();
        $this->path = $this->GetActiveBE()["mountpoint"];

        watchdog("jailadmin", "Jail @jail BE @be activated", array(
            "@jail" => $this->name,
            "@be" => $name,
        ), WATCHDOG_INFO);

        return TRUE;
    }

    public function DeactivateBE($name) {
        if ($this->IsOnline()) {
            $requested = $this->ResolveBE($name);
            if ($requested !== FALSE && $this->path == $requested["mountpoint"])
                if ($this->Stop() == FALSE) {
                    drupal_set_message(t("Could not deactivate BE @be. Jail is online and cannot be shut down.", array("@be" => $name)), "error");
                    return FALSE;
                }
        }

        $count = 0;
        foreach ($this->BEs as $be)
            if ($be["active"])
                $count++;

        if ($count < 2) {
            drupal_set_message(t("Cannot delete BE @be. You must have at least one active BE.", array("@be" => $name)));
            return FALSE;
        }

        $output = array();
        exec("/usr/local/bin/sudo /sbin/zfs set jailadmin:be_active=false {$this->dataset}/ROOT/{$name}", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Jail @jail BE @be deactivated", array(
            "@jail" => $this->name,
            "@be" => $name,
        ), WATCHDOG_INFO);

        return TRUE;
    }

    public function DeleteBE($name) {
        foreach ($this->BEs as $be) {
            if ($be["pretty_dataset"] == $name) {
                if ($be["active"]) {
                    drupal_set_message(t('Cannot delete BE @be. It is the active BE.', array('@be' => $name)), "error");
                    return FALSE;
                }

                $output = array();
                exec("/usr/local/bin/sudo /sbin/zfs destroy -r {$be["dataset"]}", $output, $res);
                if ($res != 0)
                    return FALSE;

                watchdog("jailadmin", "Jail @jail BE @be deleted", array(
                    "@jail" => $this->name,
                    "@be" => $name,
                ), WATCHDOG_INFO);

                return TRUE;
            }
        }
    }

    public function Create($template='', $usebe=FALSE) {
        $dataset = $this->dataset;
        $opts = "";

        if (strlen($template)) {
            /* If $template is set, we need to create this jail */

            if ($usebe) {
                $output = array();
                exec("/usr/local/bin/sudo /sbin/zfs create {$this->dataset}", $output, $res);
                if ($res != 0)
                    return FALSE;

                $output = array();
                exec("/usr/local/bin/sudo /sbin/zfs create {$this->dataset}/ROOT", $output, $res);
                if ($res != 0)
                    return FALSE;

                $dataset .= "/ROOT/base";

                $opts = "-o jailadmin:be_active=true";
            }

            $output = array();
            exec("/usr/local/bin/sudo zfs clone {$opts} {$template} {$dataset}", $output, $res);
            if ($res != 0)
                return FALSE;
        }

        db_insert('jailadmin_jails')
            ->fields(array(
                'name' => $this->name,
                'dataset' => $this->dataset,
                'hostname' => $this->hostname,
            ))->execute();

        watchdog("jailadmin", "Jail @jail created", array("@jail" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function Delete($destroy) {
        if ($this->IsOnline())
            $this->Stop();

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

        if ($destroy) {
            $output = array();
            exec("/usr/local/bin/sudo /sbin/zfs destroy -r {$this->dataset}", $output, $res);
            if ($res != 0)
                return FALSE;
        }

        watchdog("jailadmin", "Jail @jail deleted", array("@jail" => $this->name), WATCHDOG_INFO);

        return TRUE;
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
