<?php

class Jail {
    public $name;
    public $path;
    public $dataset;
    public $route;
    public $network;
    public $services;

    function __construct() {
        $this->network = array();
        $this->services = array();
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
        $jail->path = $record['path'];
        $jail->dataset = $record['dataset'];
        $jail->route = $record['route'];
        $jail->network = NetworkDevice::Load($jail);
        $jail->services = Service::Load($jail);

        return $jail;
    }

    public function IsOnline() {
        $o = exec("mount | grep {$this->name}/dev");
        return strlen($o) > 0;
    }

    public function IsOnlineString() {
        if ($this->IsOnline())
            return 'Online';

        return 'Offline';
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

        exec("/usr/local/bin/sudo /usr/sbin/jexec {$this->name} route add default {$this->route}");

        foreach ($this->services as $service)
            exec("/usr/local/bin/sudo /usr/sbin/jexec {$this->name} {$service->path} start");

        return TRUE;
    }

    public function Stop() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        exec("/usr/local/bin/sudo /usr/sbin/jail -r {$this->name}");
        exec("/usr/local/bin/sudo /sbin/umount {$this->path}/dev");

        foreach ($this->network as $n)
            $n->BringOffline();

        return TRUE;
    }

    public function Create() {
        db_insert('jailadmin_jails')
            ->fields(array(
                'name' => $this->name,
                'path' => $this->path,
                'dataset' => $this->dataset,
                'route' => $this->route,
            ))->execute();
    }

    public function Delete() {
        db_delete('jailadmin_jails')
            ->condition('name', $this->name)
            ->execute();
    }
}
