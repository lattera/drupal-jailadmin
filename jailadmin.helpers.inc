<?php

function get_all_jails() {
    return db_query('SELECT * FROM {jailadmin_jails}');
}

function get_all_jails_for_select() {
    $result = get_all_jails();
    $jails = array();

    foreach ($result as $record)
        $jails[$record->name] = $record->name;

    return $jails;
}
