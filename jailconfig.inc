<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_config($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $network_devices = get_all_network_devices_for_select($jail);
    $services = get_all_services_for_select($jail);
    $mounts = get_all_mounts_for_select($jail);
    $routes = get_all_routes_for_select($jail);
    $readonly = FALSE;

    if (user_access('config ' . $jail->name) == FALSE)
        $readonly = TRUE;

    if ($readonly) {
        if (user_access('view ' . $jail->name . ' config') == FALSE) {
            drupal_set_message(t('Access denied'), 'error');
            return;
        }
    }

    drupal_set_title(t('Configure @jail', array('@jail' => $jail->name)));

    if ($jail->IsOnline())
        drupal_set_message(t('NOTE: Jail is online. All settings are read-only.'), 'status');

    $form['base'] = array(
        '#type' => 'fieldset',
        '#title' => t('Base Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
    );

    $form['base']['hostname'] = array(
        '#type' => 'textfield',
        '#title' => t('Hostname'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
        "#default_value" => $jail->hostname,
    );

    $form['base']['dataset'] = array(
        '#type' => 'textfield',
        '#title' => t('ZFS Dataset'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
        '#default_value' => $jail->dataset,
    );

    $form['base']['path'] = array(
        '#type' => 'item',
        '#title' => t('Path'),
        '#markup' => $jail->path,
    );

    $form['base']['autoboot'] = array(
        '#type' => 'checkbox',
        '#title' => t('Autoboot'),
        '#description' => t('Start jail at boot?'),
        '#default_value' => $jail->autoboot,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['network_devices'] = array(
        '#type' => 'fieldset',
        '#title' => t('Network Devices'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($network_devices)) {
        $form['network_devices']['devconfig'] = array(
            '#type' => 'markup',
            '#prefix' => '<div>',
            '#markup' => l('Configure existing devices', "jailadmin/{$jail->name}/network"),
            '#suffix' => '</div>',
        );

        $form['network_devices']['devices'] = array(
            '#type' => 'select',
            '#title' => t('Network Devices'),
            '#multiple' => TRUE,
            '#options' => get_all_network_devices_for_select($jail),
            '#description' => t('Select one or more to delete'),
            '#disabled' => $jail->IsOnline() || $readonly,
        );
    }

    $form['network_devices']['new_network_device'] = array(
        '#type' => 'textfield',
        '#title' => t('New Device Name'),
        '#size' => 60,
        '#description' => t('e.g. epair@device', array('@device' => NetworkDevice::NextAvailableDevice())),
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['network_devices']['new_network_network'] = array(
        '#type' => 'select',
        '#title' => 'Network',
        '#options' => get_all_networks_for_select(),
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['network_devices']['new_network_span'] = array(
        '#type' => 'checkbox',
        '#title' => 'Span Device',
        '#description' => 'Device is a span device?',
        '#default_value' => FALSE,
        '#disabled' => $jail->isOnline() || $readonly,
    );

    $form['network_devices']['new_network_dhcp'] = array(
        '#type' => 'checkbox',
        '#title' => 'Use DHCP',
        '#default_value' => FALSE,
        '#disabled' => $jail->isOnline() || $readonly,
    );

    $form['routes'] = array(
        '#type' => 'fieldset',
        '#title' => t('Routes'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($routes)) {
        $form['routes']['existing_routes'] = array(
            '#type' => 'select',
            '#title' => t('Existing Routes'),
            '#description' => t('Select one or more to delete'),
            '#multiple' => TRUE,
            '#options' => $routes,
            '#disabled' => $jail->IsOnline() || $readonly,
        );
    }

    $form['routes']['new_route'] = array(
        '#type' => 'fieldset',
        '#title' => t('New Route'),
    );

    $form['routes']['new_route']['new_route_source'] = array(
        '#type' => 'textfield',
        '#title' => t('Source'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['routes']['new_route']['new_route_destination'] = array(
        '#type' => 'textfield',
        '#title' => t('Destination'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['services'] = array(
        '#type' => 'fieldset',
        '#title' => t('Services'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($services)) {
        $form['services']['services'] = array(
            '#type' => 'select',
            '#multiple' => TRUE,
            '#description' => t('Select one or more to delete'),
            '#options' => $services,
            '#disabled' => $jail->IsOnline() || $readonly,
        );
    }

    $form['services']['new_service'] = array(
        '#type' => 'textfield',
        '#title' => t('New Service'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['mounts'] = array(
        '#type' => 'fieldset',
        '#title' => t('Custom Mount Points'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($mounts)) {
        $form['mounts']['existing_mounts'] = array(
            '#type' => 'select',
            '#title' => t('Existing Mounts'),
            '#description' => t('Select one or more to delete'),
            '#multiple' => TRUE,
            '#options' => $mounts,
            '#disabled' => $jail->IsOnline() || $readonly,
        );
    }

    $form['mounts']['new_mount_source'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount source'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['mounts']['new_mount_target'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount target'),
        '#description' => t('Without jail path prefix. (e.g. /mnt)'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['mounts']['new_mount_driver'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount driver'),
        '#description' => t('Driver for -t argument. (e.g. nullfs)'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['mounts']['new_mount_options'] = array(
        '#type' => 'textfield',
        '#title' => t('Extra options'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    if ($jail->HasBEs) {
        $form['boot_environments'] = array(
            '#type' => 'fieldset',
            '#title' => t('Boot Environments'),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
        );

        $boot_env = array();
        foreach ($jail->BEs as $be)
            $boot_env[$be["pretty_dataset"]] = $be["pretty_dataset"] . ($be["active"] ? " (active)" : "");

        $form['boot_environments']['existing_boot_environments'] = array(
            '#type' => 'select',
            '#title' => t('Existing Boot Environments'),
            '#multiple' => TRUE,
            '#options' => $boot_env,
            '#disabled' => $readonly,
        );

        $form['boot_environments']['new_boot_environment_name'] = array(
            '#type' => 'textfield',
            '#title' => t('New BE Name'),
            '#size' => 60,
            '#disabled' => $readonly,
        );

        $form['boot_environments']['new_help'] = array(
            '#type' => 'item',
            '#markup' => t('To use a different BE than the currently-active BE as a base, select that BE from the list'),
        );

        $form['boot_environments']['create_boot_environment'] = array(
            '#type' => 'submit',
            '#value' => t('Create BE'),
            '#submit' => array('create_boot_environment'),
            '#disabled' => $readonly,
        );

        $form['boot_environments']['activate_boot_environment'] = array(
            '#type' => 'submit',
            '#value' => t('Activate BE'),
            '#submit' => array('activate_boot_environment'),
            '#disabled' => $jail->IsOnline() || $readonly,
        );

        $form['boot_environments']['delete_boot_environment'] = array(
            '#type' => 'submit',
            '#value' => t('Delete BEs'),
            '#submit' => array('delete_boot_environment'),
            '#disabled' => $readonly,
        );
    }

    if (count($jail->GetSnapshots())) {
        $form['snapshots'] = array(
            '#type' => 'fieldset',
            '#title' => t('Snapshots'),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE
        );

        $form['snapshots']['existing_snapshots'] = array(
            '#type' => 'select',
            '#multiple' => TRUE,
            '#title' => t('Existing snapshots'),
            '#options' => drupal_map_assoc($jail->GetSnapshots()),
            '#disabled' => $readonly,
        );

        $form['snapshots']['delete_snapshot'] = array(
            '#type' => 'submit',
            '#value' => t('Delete Snapshots'),
            '#submit' => array('snapshot_delete'),
            '#disabled' => $readonly,
        );

        $form['snapshots']['revert_snapshot'] = array(
            '#type' => 'submit',
            '#value' => t('Revert to Snapshot'),
            '#submit' => array('snapshot_revert'),
            '#disabled' => $jail->IsOnline() || $readonly,
        );

        $form['snapshots']['create_template'] = array(
            '#type' => 'submit',
            '#value' => t('Create Template From Snapshot'),
            '#submit' => array('create_template'),
            '#disabled' => $readonly,
        );
    }

    $form['jail_actions'] = array(
        '#type' => 'fieldset',
        '#title' => t('Jail Actions'),
    );

    $form['jail_actions']['start'] = array(
        '#type' => 'submit',
        '#value' => t('Start Jail'),
        '#submit' => array('jail_actions_start'),
        '#disabled' => $jail->IsOnline() || $readonly || (user_access("start jails") == FALSE),
    );

    $form['jail_actions']['stop'] = array(
        '#type' => 'submit',
        '#value' => t('Stop Jail'),
        '#submit' => array('jail_actions_stop'),
        '#disabled' => !$jail->IsOnline() || $readonly || (user_access("start jails") == FALSE),
    );

    $form['jail_actions']['snapshot'] = array(
        '#type' => 'submit',
        '#value' => t('Snapshot'),
        '#submit' => array('jail_actions_snapshot'),
        '#disabled' => $readonly,
    );

    $form['jail_actions']['upgrade'] = array(
        '#type' => 'submit',
        '#value' => t('Upgrade World'),
        '#submit' => array('jail_actions_upgrade'),
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save Configuration'),
        '#disabled' => $jail->IsOnline() || $readonly,
    );

    return $form;
}

function jailadmin_config_submit($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $dirty = FALSE;

    if (user_access('config ' . $jail->name) == FALSE) {
        drupal_set_message(t('Access denied'), 'error');
        $form_state['rebuild'] = TRUE;
        return;
    }

    if (isset($form_state['values']['autoboot'])) {
        $autoboot = $jail->autoboot;
        $jail->autoboot = ($form_state['values']['autoboot'] == 1);

        if ($autoboot != $jail->autoboot)
            $dirty = TRUE;
    }

    if ($form_state['values']['hostname'] != $jail->hostname) {
        $jail->hostname = $form_state['values']['hostname'];
        $dirty = TRUE;
    }

    if (isset($form_state['values']['route']) && strcmp($jail->route, $form_state['values']['route'])) {
        $jail->route = $form_state['values']['route'];
        $dirty = TRUE;
    }

    if (isset($form_state['values']['path']) && strcmp($jail->path, $form_state['values']['path'])) {
        $jail->path = $form_state['values']['path'];
        $dirty = TRUE;
    }

    if (isset($form_state['values']['dataset']) && strcmp($jail->dataset, $form_state['values']['dataset'])) {
        $jail->dataset = $form_state['values']['dataset'];
        $dirty = TRUE;
    }

    if (isset($form_state['values']['devices'])) {
        foreach(array_filter($form_state['values']['devices']) as $device) {
            $network_device = NetworkDevice::LoadByDeviceName($jail, $device);
            $network_device->Delete();
        }
    }

    if (isset($form_state['values']['existing_routes'])) {
        foreach(array_filter($form_state['values']['existing_routes']) as $route) {
            $arr = explode('^', $route);
            if (count($arr) != 3) {
                drupal_set_message(t('Improper route object sent'), 'error');
            } else {
                db_delete('jailadmin_routes')
                    ->condition('jail', $jail->name)
                    ->condition('source', $arr[0])
                    ->condition('destination', $arr[1])
                    ->execute();
            }
        }
    }

    if (isset($form_state['values']['existing_mounts'])) {
        foreach (array_filter($form_state['values']['existing_mounts']) as $target) {
            $mount = Mount::LoadByTarget($jail, $target);
            $mount->Delete();
        }
    }

    if (isset($form_state['values']['new_mount_source']) && strlen($form_state['values']['new_mount_source'])) {
        $mount = new Mount;

        $mount->jail = $jail;
        $mount->source = $form_state['values']['new_mount_source'];
        $mount->target = $form_state['values']['new_mount_target'];
        $mount->driver = $form_state['values']['new_mount_driver'];
        $mount->options = $form_state['values']['new_mount_options'];

        $mount->Create();

        $form_state['input']['new_mount_source'] = '';
        $form_state['input']['new_mount_target'] = '';
        $form_state['input']['new_mount_driver'] = '';
        $form_state['input']['new_mount_options'] = '';
    }

    if (isset($form_state['values']['new_network_device']) && strlen($form_state['values']['new_network_device'])) {
        if (strlen($form_state['values']['new_network_device'])) {
            if (NetworkDevice::IsDeviceAvailable($form_state['values']['new_network_device']) == FALSE) {
                form_set_error('new_network_device', t('Device not available'));
                $form_state['rebuild'] = TRUE;
                return;
            }
        }

        $network_device = new NetworkDevice;
        $network_device->jail = $jail;

        $network_device->bridge = Network::Load($form_state['values']['new_network_network']);
        $network_device->device = $form_state['values']['new_network_device'];
        $network_device->is_span = $form_state['values']['new_network_span'];
        $network_device->dhcp = $form_state['values']['new_network_dhcp'];

        $network_device->Create();

        $form_state['input']['new_network_device'] = '';
    }

    if (isset($form_state['values']['new_route_source']) && strlen($form_state['values']['new_route_source'])) {
        db_insert('jailadmin_routes')
            ->fields(array(
                'jail' => $jail->name,
                'source' => $form_state['values']['new_route_source'],
                'destination' => $form_state['values']['new_route_destination'],
            ))->execute();

        $form_state['input']['new_route_source'] = '';
        $form_state['input']['new_route_destination'] = '';
    }

    if (isset($form_state['values']['services'])) {
        foreach (array_filter($form_state['values']['services']) as $raw_service) {
            $service = new Service;
            $service->jail = $jail;
            $service->path = $raw_service;

            $service->Delete();
        }
    }

    if (isset($form_state['values']['new_service']) && strlen($form_state['values']['new_service'])) {
        $service = new Service;
        $service->jail = $jail;
        $service->path = $form_state['values']['new_service'];

        $service->Create();

        $form_state['input']['new_service'] = '';
    }

    if ($dirty)
        $jail->Persist();

    drupal_set_message(t('Settings saved!'));

    $form_state['rebuild']  = TRUE;
}

function create_template($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (isset($form_state['values']['existing_snapshots'])) {
        foreach (array_filter($form_state['values']['existing_snapshots']) as $snapshot) {
            $jail->CreateTemplateFromSnapshot($snapshot);
            drupal_set_message(t('Created template named "@snapshot"', array('@snapshot' => $snapshot)));
        }
    }

    $form_state['rebuild'] = TRUE;
}

function snapshot_delete($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if (isset($form_state['values']['existing_snapshots'])) {
        foreach(array_filter($form_state['values']['existing_snapshots']) as $snapshot) {
            $jail->DeleteSnapshot($snapshot);
            drupal_set_message(t('Snapshot @snapshot deleted', array('@snapshot' => $snapshot)));
        }
    }

    $form_state['rebuild'] = TRUE;
}

function snapshot_revert($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if (isset($form_state['values']['existing_snapshots'])) {
        $snapshots = array_filter($form_state['values']['existing_snapshots']);

        if (count($snapshots) == 0 || count($snapshots) > 1) {
            form_set_error('existing_snapshots', t('You must select only a single snapshot to revert to'));
            $form_state['rebuild'] = TRUE;
            return;
        }

        foreach ($snapshots as $snapshot) {
            $jail->RevertSnapshot($snapshot);
            drupal_set_message(t('@jail reverted to snapshot @snapshot', array('@jail' => $jail->name, '@snapshot' => $snapshot)));
        }
    }

    $form_state['rebuild'] = TRUE;
}

function jail_actions_start($form, &$form_state) {
    if (user_access("start jails") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Start())
        drupal_set_message(t('Jail @jail started successfully', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail failed to start', array('@jail' => $jail->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jail_actions_stop($form, &$form_state) {
    if (user_access("start jails") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Stop())
        drupal_set_message(t('Jail @jail stopped successfully', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail failed to stop', array('@jail' => $jail->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jail_actions_snapshot($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if ($jail->Snapshot() !== FALSE)
        drupal_set_message(t('Jail @jail snapshot successful', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail snapshot unsuccessful', array('@jail' => $jail->name)));

    $form_state['rebuild'] = TRUE;
}

function jail_actions_upgrade($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if ($jail->UpgradeWorld())
        drupal_set_message(t('Jail @jail upgraded successfully. NOTE: Upgrade started in background. Wait a few minutes before starting jail.', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail upgraded unsuccessfully', array('@jail' => $jail->name)));

    $form_state['rebuild'] = TRUE;
}

function create_boot_environment($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $base = "";

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state["rebuild"] = TRUE;
        return;
    }

    if (isset($form_state['values']['existing_boot_environments'])) {
        foreach(array_filter($form_state['values']['existing_boot_environments']) as $base) {
            break;
        }
    }

    if (!isset($form_state['values']['new_boot_environment_name']) || !strlen($form_state['values']['new_boot_environment_name'])) {
        drupal_set_message(t('Please fill out the new BE\'s name'), "error");
        $form['boot_environments']['#collapsed'] = FALSE;
        $form_state["rebuild"] = TRUE;
        return;
    }
    $bename = $form_state['values']['new_boot_environment_name'];

    $jail->CreateNewBE($bename, $base);

    $form_state['input']['new_boot_environment_name'] = '';
    $form_state['input']['existing_boot_environments'] = '';

    $form_state['rebuild'] = TRUE;

    drupal_set_message(t('New BE @be created successfully', array('@be' => $bename)));
}

function delete_boot_environment($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if (isset($form_state['values']['existing_boot_environments'])) {
        foreach (array_filter($form_state['values']['existing_boot_environments']) as $name) {
            if ($jail->DeleteBE($name) == FALSE) {
                drupal_set_message(t("Could not delete BE @be", array("@be" => $name)));
            }
        }
    }

    $form_state['rebuild'] = TRUE;
}

function activate_boot_environment($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        return;
    }

    if (isset($form_state['values']['existing_boot_environments'])) {
        foreach (array_filter($form_state['values']['existing_boot_environments']) as $name) {
            if ($jail->ActivateBE($name) == FALSE) {
                drupal_set_message(t("Failed to activate BE @be", array("@be" => $name)), "error");
            } else {
                drupal_set_message(t("BE @be successfully activated", array("@be" => $name)));
            }
        }
    }
}
