<?php

class NetworkDevice {
    public $device;
    public $ip;
    public $bridge;

    public static function Load($jail) {
        $result = db_query('SELECT * FROM {jailadmin_epairs} WHERE jail = :jail', array(':jail' => $jail->name));

        $devices = array();

        while ($record = $result->fetchAssoc())
            $devices[] = NetworkDevice::LoadFromRecord($record);

        return $devices;
    }

    protected static function LoadFromRecord($record=array()) {
        if (count($record) == 0)
            return FALSE;

        $net_device = new NetworkDevice;
        $net_device->device = $record['device'];
        $net_device->ip = $record['ip'];
        $net_device->bridge = Bridge::Load($record['bridge']);

        return $net_device;
    }
}
