<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function check_input($form_state=array(), $name) {
    if (isset($form_state['values'][$name]) && strlen($form_state['values'][$name]))
        return $form_state['values'][$name];

    if (isset($form_state['input'][$name]) && strlen($form_state['input'][$name]))
        return $form_state['input'][$name];

    return FALSE;
}

function jailadmin_config_network($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $devices = get_all_network_devices_for_select($jail);
    $devicename = check_input($form_state, 'device');
    $readonly = FALSE;

    if (user_access('config ' . $jail->name) == FALSE)
        $readonly = TRUE;

    if ($readonly) {
        if (user_access('view ' . $jail->name . ' config') == FALSE) {
            drupal_set_message(t('Access Denied'), 'error');
            return $form;
        }
    }

    if ($jail->IsOnline())
        drupal_set_message(t('Jail is online. All settings are read-only.'), 'status');

    $form['devices'] = array(
        '#type' => 'fieldset',
        '#title' => t('Devices'),
    );

    if (count($devices)) {
        $form['devices']['device'] = array(
            '#type' => 'select',
            '#title' => t('Existing Devices'),
            '#description' => t('Select one or more to modify'),
            '#required' => TRUE,
            '#options' => $devices,
        );

        if ($devicename !== FALSE)
            $form['devices']['device']['#default_value'] = $devicename;
    }

    if ($devicename !== FALSE) {
        $form['devices'][$devicename]['settings'] = array(
            '#type' => 'fieldset',
            '#title' => t('@device settings', array('@device' => $devicename)),
        );

        $device = NetworkDevice::LoadByDeviceName($jail, $devicename);
        $form['devices'][$devicename]['settings']['existing'] = array(
            '#type' => 'select',
            '#title' => t('Existing IPs'),
            '#description' => t('Select one or more to delete'),
            '#multiple' => TRUE,
            '#options' => drupal_map_assoc($device->ips),
            '#disabled' => $jail->IsOnline() || $readonly,
        );

        $form['devices'][$devicename]['new_ip'] = array(
            '#type' => 'textfield',
            '#title' => t('Add New IP'),
            '#size' => 60,
            '#diabled' => $jail->IsOnline() || $readonly,
        );
    }

    $form['save'] = array(
        '#type' => 'submit',
        '#value' => t('Save'),
    );

    $form_state['rebuild'] = TRUE;

    return $form;
}

function jailadmin_config_network_submit($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);
    $form_state['rebuild'] = TRUE;

    $devicename = check_input($form_state, 'device');
    if ($devicename === FALSE)
        return $form;

    if (isset($form_state['values']['existing']) && is_array($form_state['values']['existing']) && count($form_state['values']['existing'])) {
        foreach (array_filter($form_state['values']['existing']) as $todelete) {
            db_delete('jailadmin_epair_aliases')
                ->condition('device', $devicename)
                ->condition('ip', $todelete)
                ->execute();
        }
    }

    $new_ip = check_input($form_state, 'new_ip');
    if ($new_ip !== FALSE && strlen($new_ip)) {
        db_insert('jailadmin_epair_aliases')
            ->fields(array(
                'device' => $devicename,
                'ip' => $new_ip,
            ))->execute();

        $form_state['input']['new_ip'] = '';
    }

    drupal_set_message(t('Settings saved!'));

    return $form;
}
