<?php

class Mount {
    public $source;
    public $target;
    public $driver;
    public $options;
    public $jail;

    public static function Load($jail) {
        $mounts = array();

        $result = db_query('SELECT * FROM {jailadmin_mounts} WHERE jail = :jail', array(':jail' => $jail->name));

        while ($record = $result->fetchAssoc())
            $mounts[] = Mount::LoadFromRecord($jail, $record);

        return $mounts;
    }

    public static function LoadByTarget($jail, $target) {
        $result = db_query('SELECT * FROM {jailadmin_mounts} WHERE jail = :jail AND target = :target', array(':jail' => $jail->name, ':target' => $target));

        return Mount::LoadFromRecord($jail, $result->fetchAssoc());
    }

    public static function LoadFromRecord($jail, $record=array()) {
        $mount = new Mount;

        $mount->jail = $jail;
        $mount->source = $record['source'];
        $mount->target = $record['target'];
        $mount->driver = $record['driver'];
        $mount->options = $record['options'];

        return $mount;
    }

    public function Create() {
        db_insert('jailadmin_mounts')
            ->fields(array(
                'jail' => $this->jail->name,
                'source' => $this->source,
                'target' => $this->target,
                'driver' => $this->driver,
                'options' => $this->options,
            ))->execute();
    }

    public function Delete() {
        db_delete('jailadmin_mounts')
            ->condition('jail', $this->jail->name)
            ->condition('target', $this->target)
            ->execute();
    }
}
