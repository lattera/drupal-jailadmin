<?php

class Jail {
    public $name;
    public $path;
    public $dataset;
    public $routes;
    public $network;
    public $services;
    public $mounts;
    private $_snapshots;

    function __construct() {
        $this->network = array();
        $this->services = array();
        $this->_snapshots = array();
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

        $jail->path = exec("/sbin/zfs get -H -o value mountpoint {$jail->dataset}");

        return $jail;
    }

    private function load_snapshots() {
        exec("/sbin/zfs list -rH -oname -t snapshot {$this->dataset}", $this->_snapshots);
    }

    public function GetSnapshots() {
        if (count($this->_snapshots) > 0)
            return $this->_snapshots;

        $this->load_snapshots();

        return $this->_snapshots;
    }

    public function RevertSnapshot($snapshot) {
        exec("/usr/local/bin/sudo /sbin/zfs rollback -rf \"{$snapshot}\"");

        return TRUE;
    }

    public function CreateTemplateFromSnapshot($snapshot, $name='') {
        if ($name == '')
            $name = $snapshot;

        db_insert('jailadmin_templates')
            ->fields(array(
                'name' => $name,
                'snapshot' => $snapshot,
            ))
            ->execute();

        return TRUE;
    }

    public function DeleteSnapshot($snapshot) {
        exec("/usr/local/bin/sudo /sbin/zfs destroy -rf \"{$snapshot}\"");

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
            if ($n->is_span)
                $status = "(SPAN)";

            if (!count($n->ips))
                $status .= " (NO IP)";

            foreach ($n->ips as $ip)
                $status .= " {$ip}";

            $status .= ($n->IsOnline()) ? " (online)" : " (offline)";
        }

        return $status;
    }

    public function Start() {
        if ($this->IsOnline())
            if ($this->Stop() == FALSE)
                return FALSE;

        exec("/usr/local/bin/sudo /sbin/mount -t devfs devfs {$this->path}/dev");
        exec("/usr/local/bin/sudo /usr/sbin/jail -c vnet 'name={$this->name}' 'host.hostname={$this->name}' 'path={$this->path}' persist");

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

            exec("{$command} {$mount->source} {$this->path}/{$mount->target}");
        }

        foreach ($this->services as $service)
            exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->name}\" {$service->path} start");

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

    public function Snapshot() {
        $date = strftime("%F_%T");

        exec("/usr/local/bin/sudo /sbin/zfs snapshot {$this->dataset}@{$date}");

        return TRUE;
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

    public function SetupServices() {
        if (count($this->network)) {
            $ip = Network::SanitizedIP($this->network[0]->ip);
            exec("/usr/local/bin/sudo /bin/sh -c '/bin/echo \"ListenAddress {$ip}\" >> {$this->path}/etc/ssh/sshd_config'");
            exec("/usr/local/bin/sudo /bin/sh -c '/bin/echo sshd_enable=\\\"YES\\\" >> {$this->path}/etc/rc.conf'");
        }
    }

    public function Create($template='') {
        if (strlen($template)) {
            /* If $template is set, we need to create this jail */
            exec("/usr/local/bin/sudo zfs clone {$template} {$this->dataset}");
        }

        db_insert('jailadmin_jails')
            ->fields(array(
                'name' => $this->name,
                'dataset' => $this->dataset,
                'route' => $this->route,
            ))->execute();
    }

    public function Delete($destroy) {
        foreach ($this->network as $n)
            $n->Delete();

        foreach ($this->services as $s)
            $s->Delete();

        foreach ($this->mounts as $m)
            $m->Delete();

        db_delete('jailadmin_jails')
            ->condition('name', $this->name)
            ->execute();

        if ($destroy)
            exec("/usr/local/bin/sudo /sbin/zfs destroy {$this->dataset}");
    }

    public function Persist() {
        db_update('jailadmin_jails')
            ->fields(array(
                'route' => $this->route,
                'dataset' => $this->dataset,
            ))
            ->condition('name', $this->name)
            ->execute();
    }
}
