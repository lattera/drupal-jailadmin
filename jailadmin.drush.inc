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
        'jail' => array(
            'description' => 'Get/set jail settings',
            'arguments' => array(
                'jailname' => 'Name of the jail',
                'variable' => 'Name of the variable to get or set',
                'setting' => 'Optional value to set the variable to',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
        'clone_jail' => array(
            'description' => 'Clone a jail',
            'arguments' => array(
                'jailname' => 'Name of the jail to clone',
                'newname' => 'Name of the new jail',
                'newdataset' => 'ZFS dataset of the new jail',
                'hostname' => 'Hostname of the new jail',
            ),
            'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
        ),
    );
}

function drush_jailadmin_autoboot() {
    global $user;

    $user = user_load(1);

    $jails = Jail::LoadAll();

    foreach ($jails as $jail) {
        if ($jail->autoboot) {
            $jail->Start();
        }
    }
}

function drush_jailadmin_start($jailname='') {
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

    $jails = Jail::LoadAll();
    foreach ($jails as $jail)
        if ($jail->IsOnline())
            $jail->Stop();
}

function drush_jailadmin_jail_status($jailname='') {
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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
    global $user;

    $user = user_load(1);

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

function drush_jailadmin_jail($jailname='', $variable='', $setting='') {
    global $user;

    $user = user_load(1);

    if ($jailname == '') {
        drush_print("Please provide a jail name");
        return;
    }

    if ($variable == '') {
        drush_print("Please provide the option to get/set");
        return;
    }

    $jail = Jail::Load($jailname);
    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    if ($setting != '')
        return set_jail_var($jail, $variable, $setting);

    switch ($variable) {
        case "path":
            drush_print("{$jail->path}");
            break;
        case "dataset":
            drush_print("{$jail->dataset}");
            break;
        case "autoboot":
            drush_print("{$jail->autoboot}");
            break;
        case "active_BE":
            if (!$jail->HasBEs) {
                drush_print("Jail not configured for BEs");
                return;
            }

            drush_print($jail->GetActiveBE()["pretty_dataset"]);
            break;
        default:
            drush_print("Unknown setting");
            break;
    }
}

function set_jail_var($jail, $variable, $setting)
{
    switch ($variable) {
        case "dataset":
            $jail->dataset = $setting;
            break;

        case "autoboot":
            switch (substr(strtolower($setting), 0, 1)) {
                case "y":
                case "t":
                case "1":
                    $jail->autoboot = true;
                    break;
                default:
                    $jail->autoboot = false;
                    break;
            }

            break;

        case "active_BE":
            $jail->ActivateBE($setting);
            break;
    }

    $jail->Persist();
}

function drush_jailadmin_clone_jail($jailname='', $newname='', $newdataset='', $newhostname='') {
    global $user;

    $user = user_load(1);

    if ($jailname == '') {
        drush_print("Please specify a valid jail name");
        return;
    }

    if ($newname == '') {
        drush_print("Please specify the name of the new jail");
        return;
    }

    if ($newdataset == '') {
        drush_print("Please specify the dataset of the new jail");
        return;
    }

    $jail = Jail::Load($jailname);

    if ($jail === FALSE) {
        drush_print("Jail not found");
        return;
    }

    if ($jail->CloneJail($newname, $newdataset, $newhostname) == FALSE)
        drush_print("Could not clone jail");
    else
        drush_print("Clone successful");
}
