<?php

function get_all_jails_for_select() {
    $all = Jail::LoadAll();
    $jails = array();

    foreach ($all as $jail)
        $jails[$jail->name] = $jail->name;

    return $jails;
}
