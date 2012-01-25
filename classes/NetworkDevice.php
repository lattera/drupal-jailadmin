<?php

class NetworkDevice {
    public $device;
    public $ip;
    public $bridge;
    public $jail;

    public static function Load($jail) {
        $result = db_query('SELECT * FROM {jailadmin_epairs} WHERE jail = :jail', array(':jail' => $jail->name));

        $devices = array();

        while ($record = $result->fetchAssoc())
            $devices[] = NetworkDevice::LoadFromRecord($jail, $record);

        return $devices;
    }

    public static function LoadByDeviceName($jail, $name) {
        $result = db_query('SELECT * FROM {jailadmin_epairs} WHERE device = :device', array(':device' => $name));

        $record = $result->fetchAssoc();
        return NetworkDevice::LoadFromRecord($jail, $record);
    }

    public function IsOnline() {
        $o = exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device}a 2>&1 | grep -v \"does not exist\"");
        return strlen($o) > 0;
    }

    public function BringHostOnline() {
        if ($this->IsOnline())
            return TRUE;

        if ($this->bridge->BringOnline() == FALSE)
            return FALSE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device} create");
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->bridge->device} addm {$this->device}a");
        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device}a up");

        return TRUE;
    }

    public function BringGuestOnline() {
        if ($this->jail->IsOnline() == FALSE)
            return FALSE;

        if ($this->IsOnline() == FALSE)
            if ($this->BringHostOnline() == FALSE)
                return FALSE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device}b vnet {$this->jail->name}");
        exec("/usr/local/bin/sudo /usr/sbin/jexec {$this->jail->name} ifconfig {$this->device}b {$this->ip}");

        return TRUE;
    }

    public function BringOffline() {
        if ($this->IsOnline() == FALSE)
            return TRUE;

        exec("/usr/local/bin/sudo /sbin/ifconfig {$this->device}a destroy");

        return TRUE;
    }

    protected static function LoadFromRecord($jail, $record=array()) {
        if (count($record) == 0)
            return FALSE;

        $net_device = new NetworkDevice;
        $net_device->device = $record['device'];
        $net_device->ip = $record['ip'];
        $net_device->bridge = Network::Load($record['bridge']);
        $net_device->jail = $jail;

        return $net_device;
    }

    public function Create() {
        db_insert('jailadmin_epairs')
            ->fields(array(
                'jail' => $this->jail->name,
                'device' => $this->device,
                'bridge' => $this->bridge->name,
                'ip' => $this->ip,
            ))->execute();
    }

    public function Delete() {
        db_delete('jailadmin_epairs')
            ->condition('device', $this->device)
            ->condition('jail', $this->jail->name)
            ->execute();
    }
}
