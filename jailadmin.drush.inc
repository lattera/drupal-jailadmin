<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_drush_command() {
    return array(
        'start' => array(
            'description' => 'Boot up a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail to boot up',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'stop' => array(
            'description' => 'Shutdown a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail to shutdown',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'autoboot' => array(
            'description' => 'Autoboot the jails',
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'stopall' => array(
            'description' => 'Stop all jails',
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'jail_status' => array(
            'description' => 'Display the status of all jails',
            'arguments' => array(
                'jailname' => 'Optional name of the jail to display',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
    );
}

function drush_jailadmin_autoboot() {
    $jails = Jail::LoadAll();

    foreach ($jails as $jail) {
        if ($jail->autoboot) {
            $jail->Start();
        }
    }
}

function drush_jailadmin_start($jailname='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }
    
    $jail->Start();
}

function drush_jailadmin_stop($jailname='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }
    
    $jail->Stop();
}

function drush_jailadmin_stopall() {
    $jails = Jail::LoadAll();
    foreach ($jails as $jail)
        if ($jail->IsOnline())
            $jail->Stop();
}

function drush_jailadmin_jail_status($jailname='') {
    $jails = array();

    if ($jailname == '')
        $jails = Jail::LoadAll();
    else
        $jails[] = Jail::Load($jailname);

    foreach ($jails as $jail) {
        if ($jail === FALSE) {
            drush_print("Jail {$jailname} not found");
            continue;
        }

        $online = $jail->IsOnline() ? "Yes" : "No";

        drush_print("{$jail->name}[Online] => {$online}");
        if ($jail->IsOnline())
            drush_print("{$jail->name}[Network] => " . $jail->NetworkStatus());
    }
}
