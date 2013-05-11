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
        'list_snapshots' => array(
            'description' => 'List all snapshots of a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'snapshot_jail' => array(
            'description' => 'Create a snapshot of a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail to snapshot',
                'base' => 'Name of the BE to snapshot (only if using BEs)',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'delete_snapshot' => array(
            'description' => 'Create a snapshot of a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail to snapshot',
                'snapshot' => 'Name of the snapshot',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'list_BEs' => array(
            'description' => 'List all BEs',
            'arguments' => array(
                'jailname' => 'Name of the jail',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'create_BE' => array(
            'description' => 'Create new BE',
            'arguments' => array(
                'jailname' => 'Name of the jail',
                'bename' => 'Name of the new BE',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'activate_BE' => array(
            'description' => 'Activate a BE',
            'arguments' => array(
                'jailname' => 'Name of the jail',
                'bename' => 'Name of the BE to activate',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'delete_BE' => array(
            'description' => 'Delete a BE',
            'arguments' => array(
                'jailname' => 'Name of the jail',
                'bename' => 'Name of the BE to delete',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'deactivate_BE' => array(
            'description' => 'Delete a BE',
            'arguments' => array(
                'jailname' => 'Name of the jail',
                'bename' => 'Name of the BE to deactivate',
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

        if ($jail->HasBEs) {
            drush_print("{$jail->name}[Active BE] => " . $jail->GetActiveBE()["pretty_dataset"]);
        }

        drush_print("{$jail->name}[Path] => {$jail->path}");

        if ($jail->IsOnline())
            drush_print("{$jail->name}[Network] => " . $jail->NetworkStatus());
    }
}

function drush_jailadmin_list_snapshots($jailname='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    foreach ($jail->GetSnapshots() as $snapshot) {
        drush_print($snapshot);
    }
}

function drush_jailadmin_snapshot_jail($jailname='', $base='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->Snapshot($base);
}

function drush_jailadmin_delete_snapshot($jailname='', $snapshot='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($snapshot == '') {
        drush_print("Please provide the snapshot name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->DeleteSnapshot($snapshot);
}

function drush_jailadmin_list_BEs($jailname) {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    if (!$jail->HasBEs) {
        drush_print("Jail not configured for BEs");
        return;
    }

    foreach ($jail->BEs as $be) {
        drush_print($be["pretty_dataset"] . " => " . $be["mountpoint"] . ($be["active"] ? " (active)" : ""));
    }
}

function drush_jailadmin_create_BE($jailname='', $bename='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($bename == '') {
        drush_print("Please provide the BE name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->CreateNewBE($bename);
}

function drush_jailadmin_activate_BE($jailname='', $bename='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($bename == '') {
        drush_print("Please provide the BE name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->ActivateBE($bename);
}

function drush_jailadmin_delete_BE($jailname='', $bename='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($bename == '') {
        drush_print("Please provide the BE name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->DeleteBE($bename);
}

function drush_jailadmin_deactivate_BE($jailname='', $bename='') {
    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($bename == '') {
        drush_print("Please provide the BE name");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    $jail->DeactivateBE($bename);
}
