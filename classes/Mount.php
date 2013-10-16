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

    public function DoMount() {
        $command = "/usr/local/bin/sudo /sbin/mount ";
        if (strlen($this->driver))
            $command .= "-t {$this->driver} ";
        if (strlen($this->options))
            $command .= "-o {$this->options} ";

        if (!is_dir("{$this->jail->path}/{$this->target}")) {
            $output = array();
            $res = 0;
            exec("/usr/local/bin/sudo /bin/mkdir -p '{$this->jail->path}/{$this->target}'", $output, $res);
            if ($res != 0) {
            watchdog("jailadmin", "Creation of mountpoint @mount in jail @jail: @reason", array(
                "@jail" => $this->jail->name,
                "@mount" => "{$this->jail->path}{$this->target}",
                "@reason" => var_export($output, TRUE)
            ), WATCHDOG_ERROR);

                return FALSE;
            }
        }

        $output = array();
        $res = 0;
        exec("{$command} {$this->source} {$this->jail->path}/{$this->target}", $output, $res);
        if ($res != 0) {
            watchdog("jailadmin", "Failed mounting @mount in jail @jail: @reason", array(
                "@jail" => $this->jail->name,
                "@mount" => $this->target,
                "@reason" => var_export($output, TRUE)
            ), WATCHDOG_ERROR);

            return FALSE;
        }

        watchdog("jailadmin", "Mounted @mount in jail @jail", array(
            "@mount" => $this->target,
            "@jail" => $this->jail->name,
        ), WATCHDOG_INFO);

        return TRUE;
    }

    public function Unmount() {
        $output = array();
        $command = "/usr/local/bin/sudo /sbin/umount ";

        exec("{$command} -f {$this->jail->path}/{$this->target}", $output, $res);
        if ($res != 0)
            return FALSE;

        watchdog("jailadmin", "Unmounted @target from jail @jail", array(
            "@target" => $this->target,
            "@jail" => $this->jail->name,
        ), WATCHDOG_INFO);

        return TRUE;
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
