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

    foreach ($all as $network) {
        $ip = "";
        
        if (!count($network->ips))
            $ip = "NO IP";

        foreach ($network->ips as $rawip) {
            if (strlen($ip))
                $ip .= ", ";

            $ip .= (strlen($rawip)) ? $rawip : "NO IP";
        }

        $networks[$network->name] = $network->name . ' (' . $ip . ')';
    }
    
    return $networks;
}

function get_all_network_devices_for_select($jail) {
    $networks = array();

    foreach ($jail->network as $network) {
        $ip = "";

        if ($network->is_span)
            $ip .= "(SPAN) ";

        if (!count($network->ips))
            $ip .= "(NO IP)";

        foreach ($network->ips as $rawip) {
            if (strlen($ip))
                $ip .= ", ";

            $ip .= (strlen($rawip)) ? $rawip : "NO IP";
        }

        $networks[$network->device] = t($network->device . ' (' . $ip . ')');
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

function get_all_mounts_for_select($jail) {
    $mounts = array();

    foreach ($jail->mounts as $mount)
        $mounts[$mount->target] = $mount->target;

    return $mounts;
}

function get_all_routes_for_select($jail) {
    $routes = array();

    $i=0;
    foreach ($jail->routes as $route)
        $routes[$route['source'] . '^' . $route['destination'] . '^' . $i++] = $route['source'] . '->' . $route['destination'];

    return $routes;
}
