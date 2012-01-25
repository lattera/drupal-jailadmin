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

function get_all_network_devices_for_select($jail) {
    $networks = array();

    foreach ($jail->network as $network) {
        $networks[$network->device] = t($network->device . ' (' . $network->ip . ')');
    }

    return $networks;
}

function get_all_services_for_select($jail) {
    $services = array();

    foreach ($jail->services as $service)
        $services[$service->path] = $service->path;

    return $services;
}

function get_all_templates_for_select() {
    $result = db_query('SELECT * FROM {jailadmin_templates}');
    $templates = array();
    $templates[] = '[NO TEMPLATE]';

    foreach ($result as $record)
        $templates[$record->snapshot] = $record->name;

    return $templates;
}
