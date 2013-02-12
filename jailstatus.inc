<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_status($form, &$form_state) {
    $jails = Jail::LoadAll();

    $header = array('Jail Name', 'Status', 'Network Status', 'Actions');
    $rows = array();
    foreach ($jails as $jail) {
        if (user_access('config ' . $jail->name) || user_access('view ' . $jail->name . ' config'))
            $name = l(t($jail->name), 'jailadmin/' . $jail->name . '/config');
        else
            $name = t('@name', array('@name' => $jail->name));

        $action = "";
        if ($jail->IsOnline())
            $action = l("Stop", "jailadmin/{$jail->name}/stop");
        else
            $action = l("Start", "jailadmin/{$jail->name}/start");

        $rows[] = array($name, $jail->IsOnlineString(), $jail->NetworkStatus(),  $action);
    }

    $form['jails'] = array(
        '#type' => 'markup',
        '#prefix' => '<div>',
        '#markup' => theme('table', array(
            'header' => $header,
            'rows' => $rows,
        )),
        '#suffix' => '</div>',
    );

    return $form;
}

function jail_start($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Start()) {
        drupal_set_message(t('Jail @jail started successfully', array('@jail' => $jail->name)));
    } else {
        drupal_set_message(t('Jail @jail failed to start', array('@jail' => $jail->name)), 'error');
    }

    $form_state['rebuild'] = true;

    return jailadmin_status($form, $form_state);
}

function jail_stop($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if ($jail->Stop()) {
        drupal_set_message(t('Jail @jail stopped successfully', array('@jail' => $jail->name)));
    } else {
        drupal_set_message(t('Jail @jail failed to stop', array('@jail' => $jail->name)), 'error');
    }

    $form_state['rebuild'] = true;

    return jailadmin_status($form, $form_state);
}
