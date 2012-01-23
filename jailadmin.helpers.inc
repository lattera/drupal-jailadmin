<?php

function get_all_jails_for_select() {
    $all = Jail::LoadAll();
    $jails = array();

    foreach ($all as $jail)
        $jails[$jail->name] = $jail->name;

    return $jails;
}

function get_all_networks_for_select() {
    $all = Network::LoadAll();
    $networks = array();

    foreach ($all as $network)
        $networks[$network->name] = $network->name . ' (' . $network->ip . ')';
    
    return $networks;
}
