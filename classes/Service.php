<?php

class Service {
    public $path;
    public $jail;

    public static function Load($jail) {
        $services = array();

        $result = db_query('SELECT * FROM {jailadmin_services} WHERE jail = :jail', array(':jail' => $jail->name));

        while ($record = $result->fetchAssoc())
            $services[] = Service::LoadFromRecord($jail, $record);

        return $services;
    }

    public static function LoadFromRecord($jail, $record=array()) {
        $service = new Service;

        $service->path = $record['path'];
        $service->jail = $jail;

        return $service;
    }

    public function Start() {
        $output = array();
        exec("/usr/local/bin/sudo /usr/sbin/jexec \"{$this->jail->name}\" {$this->path} start", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Service @service started in jail @jail", array(
            "@service" => $service,
            "@jail" => $this->name,
        ), WATCHDOG_INFO);

        return TRUE;
    }

    public function Create() {
        db_insert('jailadmin_services')
            ->fields(array(
                'path' => $this->path,
                'jail' => $this->jail->name,
            ))->execute();
    }

    public function Delete() {
        db_delete('jailadmin_services')
            ->condition('jail', $this->jail->name)
            ->condition('path', $this->path)
            ->execute();
    }
}
