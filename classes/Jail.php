<?php

class Jail {
    public $name;
    public $path;
    public $dataset;
    public $route;

    public static function Load($name) {
        $result = db_query('SELECT * FROM {jailadmin_jails} WHERE name = :name', array(':name' => $name));

        $record = $result->fetchAssoc();
        $j = new Jail;

        $j->name = $record['name'];
        $j->path = $record['path'];
        $j->dataset = $record['dataset'];
        $j->route = $record['route'];

        return $j;
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
