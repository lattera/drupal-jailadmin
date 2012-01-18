<?php

class Jail {
    public $name;
    public $path;
    public $dataset;
    public $route;
    public $network;

    function __construct() {
        $this->network = array();
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
        $jail->devices = NetworkDevice::Load($jail);

        return $jail;
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
