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
