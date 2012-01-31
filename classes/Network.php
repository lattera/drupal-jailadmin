<?php

class Network {
    public $name;
    public $device;
    public $ip;
    public $physicals;

    public function __construct() {
        $this->physicals = array();
    }

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

        /* Load physical devices to add to the bridge */
        $result = db_query('SELECT device FROM {jailadmin_bridge_physicals} WHERE bridge = :bridge', array(':bridge' => $network->name));
        foreach ($result as $physical)
            $network->physicals[] = $physical->device;

        return $network;
    }

    public static function IsIPAvailable($ip) {
        $result = db_query('SELECT ip FROM {jailadmin_bridges} WHERE CHAR_LENGTH(ip) > 0');

        foreach ($result as $record)
            if (!strcmp($record->ip, $ip))
                return FALSE;

        $result = db_query('SELECT ip FROM {jailadmin_epairs} WHERE CHAR_LENGTH(ip) > 0');

        foreach ($result as $record)
            if (!strcmp($record->ip, $ip))
                return FALSE;

        return TRUE;
    }

    public static function IsDeviceAvailable($device) {
        $result = db_query('SELECT device FROM {jailadmin_bridges}');

        foreach ($result as $record)
            if (!strcmp($record->device, $device))
                return FALSE;

        return TRUE;
    }

    public function IsOnline() {
        $o = exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} 2>&1 | grep -v \"does not exist\"");
        return strlen($o) > 0;
    }

    public function BringOnline() {
        if ($this->IsOnline())
            return TRUE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} create 2>&1");

        if (strlen($this->ip))
            exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} {$this->ip}");

        foreach ($this->physicals as $physical)
            exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} addm {$physical}");

        return TRUE;
    }

    public function BringOffline() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} destroy");

        return TRUE;
    }

    public function Persist() {
        if (Network::IsIPAvailable($this->ip) == FALSE)
            return FALSE;

        db_update('jailadmin_bridges')
            ->fields(array(
                'ip' => $this->ip,
                'device' => $this->device,
            ))
            ->condition('name', $this->name)
            ->execute();

        return TRUE;
    }

    public function Create() {
        if (Network::IsIPAvailable($this->ip) == FALSE)
            return FALSE;

        db_insert('jailadmin_bridges')
            ->fields(array(
                'name' => $this->name,
                'device' => $this->device,
                'ip' => $this->ip,
            ))->execute();

        return TRUE;
    }

    public function Delete() {
        db_delete('jailadmin_bridges')
            ->condition('name', $this->name)
            ->execute();
    }
}
