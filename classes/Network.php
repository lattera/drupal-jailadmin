<?php

class Network {
    public $name;
    public $device;
    public $ips;
    public $physicals;
    private $_jails;

    public function __construct() {
        $this->physicals = array();
        $this->_jails = NULL;
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

        /* Load physical devices to add to the bridge */
        $result = db_query('SELECT device FROM {jailadmin_bridge_physicals} WHERE bridge = :bridge', array(':bridge' => $network->name));
        foreach ($result as $physical)
            $network->physicals[] = $physical->device;

        $result = db_select('jailadmin_bridge_aliases', 'jba')
            ->fields('jba', array('ip'))
            ->condition('device', $network->device)
            ->execute();

        $network->ips = array();
        foreach ($result as $ip)
            $network->ips[] = $ip->ip;

        return $network;
    }

    public static function IsDeviceAvailable($device) {
        $result = db_query('SELECT device FROM {jailadmin_bridges}');

        foreach ($result as $record)
            if (!strcmp($record->device, $device))
                return FALSE;

        return TRUE;
    }

    public function IsOnline() {
        $output = array();
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} 2>&1", $output, $res);
        return $res == 0;
    }

    public function BringOnline() {
        if ($this->IsOnline())
            return TRUE;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} create 2>&1", $output, $res);
        if ($res != 0)
            return FALSE;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} up 2>&1", $output, $res);
        if ($res != 0)
            return FALSE;

        foreach ($this->ips as $ip) {
            $inet = (strstr($ip, ':') === FALSE) ? 'inet' : 'inet6';

            $output = array();
            exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} {$inet} {$ip} alias", $output, $res);
            if ($res != 0)
                return FALSE;
        }

        foreach ($this->physicals as $physical) {
            $output = array();
            exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} addm {$physical}", $output, $res);
            if ($res != 0)
                return FALSE;
        }

        watchdog("jailadmin", "Network @network online", array("@network" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function BringOffline() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        $output = array();
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} destroy", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Network @network offline", array("@network" => $this->name), WATCHDOG_INFO);

        return TRUE;
    }

    public function Persist() {
        db_update('jailadmin_bridges')
            ->fields(array(
                'device' => $this->device,
            ))
            ->condition('name', $this->name)
            ->execute();

        return TRUE;
    }

    public function Create() {
        db_insert('jailadmin_bridges')
            ->fields(array(
                'name' => $this->name,
                'device' => $this->device,
            ))->execute();

        return TRUE;
    }

    public function Delete() {
        db_delete('jailadmin_bridges')
            ->condition('name', $this->name)
            ->execute();

        db_delete('jailadmin_bridge_aliases')
            ->condition('device', $this->device)
            ->execute();
    }

    public function AddIP($ip) {
        db_insert('jailadmin_bridge_aliases')
            ->fields(array(
                'device' => $this->device,
                'ip' => $ip
            ))->execute();

        $this->ips[] = $ip;
    }

    public function AssignedJails() {
        if ($this->_jails != NULL)
            return $this->_jails;

        $this->_jails = array();

        $jails = Jail::LoadAll();
        foreach ($jails as $jail)
            foreach ($jail->network as $network)
                if (!strcmp($network->bridge->name, $this->name))
                    $this->_jails[] = $jail;

        return $this->_jails;
    }

    public function CanBeModified() {
        $jails = $this->AssignedJails();
        foreach ($jails as $jail)
            if ($jail->IsOnline())
                return FALSE;

        return TRUE;
    }

    public static function SanitizedIP($ip) {
        $result = strstr($ip, '/', TRUE);
        if ($result === FALSE)
            return $ip;

        return $result;
    }
}
