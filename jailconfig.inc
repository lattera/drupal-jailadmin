<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_config($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $network_devices = get_all_network_devices_for_select($jail);

    $form['base'] = array(
        '#type' => 'fieldset',
        '#title' => t('Base Settings'),
    );

    $form['base']['dataset'] = array(
        '#type' => 'textfield',
        '#title' => t('ZFS Dataset'),
        '#size' => 60,
        '#default_value' => $jail->dataset,
    );

    $form['base']['path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path'),
        '#size' => 60,
        '#default_value' => $jail->path,
    );

    $form['base']['route'] = array(
        '#type' => 'textfield',
        '#title' => t('Default Route'),
        '#size' => 60,
        '#default_value' => $jail->route,
    );

    $form['network_devices'] = array(
        '#type' => 'fieldset',
        '#title' => t('Network Devices'),
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
