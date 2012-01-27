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

    $form['base'] = array(
        '#type' => 'fieldset',
        '#title' => t('Base Settings'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
    );

    $form['base']['dataset'] = array(
        '#type' => 'textfield',
        '#title' => t('ZFS Dataset'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline(),
        '#default_value' => $jail->dataset,
    );

    $form['base']['path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline(),
        '#default_value' => $jail->path,
    );

    $form['base']['route'] = array(
        '#type' => 'textfield',
        '#title' => t('Default Route'),
        '#size' => 60,
        '#disabled' => $jail->IsOnline(),
        '#default_value' => $jail->route,
    );

    $form['network_devices'] = array(
        '#type' => 'fieldset',
        '#title' => t('Network Devices'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );

    if (count($network_devices)) {
        $form['network_devices']['devices'] = array(
            '#type' => 'select',
            '#title' => t('Network Devices'),
            '#multiple' => TRUE,
            '#options' => get_all_network_devices_for_select($jail),
            '#description' => t('Select one or more to delete'),
        );
    }

    $form['network_devices']['new_network_device'] = array(
        '#type' => 'textfield',
        '#title' => t('New Device Name'),
        '#size' => 60,
        '#description' => t('e.g. epair0'),
    );

    $form['network_devices']['new_network_ip'] = array(
        '#type' => 'textfield',
        '#title' => t('New IP'),
        '#size' => 60,
        '#description' => t('IPv4 or IPv6 IP'),
    );

    $form['network_devices']['new_network_network'] = array(
        '#type' => 'select',
        '#title' => 'Network',
        '#options' => get_all_networks_for_select(),
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
        );
    }

    $form['services']['new_service'] = array(
        '#type' => 'textfield',
        '#title' => t('New Service'),
        '#size' => 60,
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
        );
    }

    $form['mounts']['new_mount_source'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount source'),
        '#size' => 60,
    );

    $form['mounts']['new_mount_target'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount target'),
        '#description' => t('Without jail path prefix. (e.g. /mnt)'),
        '#size' => 60,
    );

    $form['mounts']['new_mount_driver'] = array(
        '#type' => 'textfield',
        '#title' => t('New mount driver'),
        '#description' => t('Driver for -t argument. (e.g. nullfs)'),
        '#size' => 60,
    );

    $form['mounts']['new_mount_options'] = array(
        '#type' => 'textfield',
        '#title' => t('Extra options'),
        '#size' => 60,
    );

    $form['jail_actions'] = array(
        '#type' => 'fieldset',
        '#title' => t('Jail Actions'),
    );

    $form['jail_actions']['start'] = array(
        '#type' => 'submit',
        '#value' => t('Start Jail'),
        '#submit' => array('jail_actions_start'),
        '#disabled' => $jail->IsOnline(),
    );

    $form['jail_actions']['stop'] = array(
        '#type' => 'submit',
        '#value' => t('Stop Jail'),
        '#submit' => array('jail_actions_stop'),
        '#disabled' => !$jail->IsOnline(),
    );

    $form['jail_actions']['snapshot'] = array(
        '#type' => 'submit',
        '#value' => t('Snapshot'),
        '#submit' => array('jail_actions_snapshot'),
    );

    $form['jail_actions']['upgrade'] = array(
        '#type' => 'submit',
        '#value' => t('Upgrade World'),
        '#submit' => array('jail_actions_upgrade'),
        '#disabled' => $jail->IsOnline(),
    );

    $form['jail_actions']['setup_services'] = array(
        '#type' => 'submit',
        '#value' => t('Setup Services'),
        '#submit' => array('jail_actions_setup_services'),
        '#disabled' => $jail->IsOnline(),
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save Configuration'),
    );

    return $form;
}

function jailadmin_config_submit($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (isset($form_state['values']['devices'])) {
        foreach(array_filter($form_state['values']['devices']) as $device) {
            $network_device = NetworkDevice::LoadByDeviceName($jail, $device);
            $network_device->Delete();
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
        $network_device = new NetworkDevice;
        $network_device->jail = $jail;

        $network_device->bridge = Network::Load($form_state['values']['new_network_network']);
        $network_device->device = $form_state['values']['new_network_device'];
        $network_device->ip = $form_state['values']['new_network_ip'];

        $network_device->Create();

        $form_state['input']['new_network_device'] = '';
        $form_state['input']['new_network_ip'] = '';
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

    $form_state['rebuild']  = TRUE;
}

function jail_actions_start($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Start())
        drupal_set_message(t('Jail @jail started successfully', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail failed to start', array('@jail' => $jail->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jail_actions_stop($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Stop())
        drupal_set_message(t('Jail @jail stopped successfully', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail failed to stop', array('@jail' => $jail->name)), 'error');

    $form_state['rebuild'] = TRUE;
}

function jail_actions_snapshot($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Snapshot())
        drupal_set_message(t('Jail @jail snapshot successful', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail snapshot unsuccessful', array('@jail' => $jail->name)));

    $form_state['rebuild'] = TRUE;
}

function jail_actions_upgrade($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->UpgradeWorld())
        drupal_set_message(t('Jail @jail upgraded successfully. NOTE: Upgrade started in background. Wait a few minutes before starting jail.', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail upgraded unsuccessfully', array('@jail' => $jail->name)));

    $form_state['rebuild'] = TRUE;
}

function jail_actions_setup_services($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    $jail->SetupServices();

    $form_state['rebuild'] = TRUE;
}
