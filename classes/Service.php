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
