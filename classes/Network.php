<?php

class Network {
    public $name;
    public $device;
    public $ip;

    public static function Load($name) {
        $result = db_query('SELECT * FROM {jailadmin_bridges} WHERE name = :name', array(':name' => $name));

        return Network::LoadFromRecord($result->fetchAssoc());
    }

    public static function LoadAll() {
        $result = db_query('SELECT * FROM {jailadmin_bridges}');
        $networks = array();

        while ($record = $result->fetchAssoc())
            $networks[] = Network::LoadFromRecord($record);

        return $networks;
    }

    public static function LoadFromRecord($record=array()) {
        $network = new Network;

        $network->name = $record['name'];
        $network->device = $record['device'];
        $network->ip = $record['ip'];

        return $network;
    }

    public function IsOnline() {
        $o = exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} 2>&1 | grep -v \"does not exist\"");
        return strlen($o) > 0;
    }

    public function BringOnline() {
        if ($this->IsOnline())
            return TRUE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} create 2>&1");
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} {$this->ip}");

        return TRUE;
    }

    public function BringOffline() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} destroy");

        return TRUE;
    }

    public function Persist() {
        db_update('jailadmin_bridges')
            ->fields(array(
                'ip' => $this->ip,
                'device' => $this->device,
            ))
            ->condition('name', $this->name)
            ->execute();
    }

    public function Create() {
        db_insert('jailadmin_bridges')
            ->fields(array(
                'name' => $this->name,
                'device' => $this->device,
                'ip' => $this->ip,
            ))->execute();
    }

    public function Delete() {
        db_delete('jailadmin_bridges')
            ->condition('name', $this->name)
            ->execute();
    }
}
