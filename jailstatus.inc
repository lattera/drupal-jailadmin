<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_status($form, &$form_state) {
    $jails = Jail::LoadAll();

    $header = array('Jail Name', 'Hostname', 'Status', 'Network Status', 'Actions');
    $rows = array();
    foreach ($jails as $jail) {
        if (user_access('config ' . $jail->name) || user_access('view ' . $jail->name . ' config'))
            $name = l(t($jail->name), 'jailadmin/' . $jail->name . '/config');
        else
            $name = t('@name', array('@name' => $jail->name));

        $action = "";
        if (user_access("start jails")) {
            if ($jail->IsOnline())
                $action = l("Stop", "jailadmin/{$jail->name}/stop");
            else
                $action = l("Start", "jailadmin/{$jail->name}/start");
        }

        if (user_access("config {$jail->name}")) {
            $network = l(t("Network Settings"), "jailadmin/{$jail->name}/network");

            $action .= (strlen($action) ? " | " : "")
                . l("Snapshot", "jailadmin/{$jail->name}/snapshot")
                . " | {$network}";
        }

        $hostname = (strlen($jail->hostname)) ? $jail->hostname : $jail->name;
        $rows[] = array($name, $hostname, $jail->IsOnlineString(), $jail->NetworkStatus(),  $action);
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

    if (user_access("start jails") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        drupal_goto("jailadmin/status");
        return;
    }

    if ($jail->Start()) {
        drupal_set_message(t('Jail @jail started successfully', array('@jail' => $jail->name)));
    } else {
        drupal_set_message(t('Jail @jail failed to start', array('@jail' => $jail->name)), 'error');
    }

    $form_state['rebuild'] = true;

    drupal_goto("jailadmin/status");
}

function jail_stop($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("start jails") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        drupal_goto("jailadmin/status");
        return;
    }

    if ($jail->Stop()) {
        drupal_set_message(t('Jail @jail stopped successfully', array('@jail' => $jail->name)));
    } else {
        drupal_set_message(t('Jail @jail failed to stop', array('@jail' => $jail->name)), 'error');
    }

    $form_state['rebuild'] = true;

    drupal_goto("jailadmin/status");
}

function jail_snapshot($form, &$form_state) {
    $jail = Jail::Load($form_state['build_info']['args'][0]);

    if (user_access("config {$jail->name}") == FALSE) {
        drupal_set_message(t("Access Denied"), "error");
        $form_state['rebuild'] = TRUE;
        drupal_goto("jailadmin/status");
        return;
    }

    if ($jail->Snapshot() !== FALSE)
        drupal_set_message(t('Jail @jail snapshot successful', array('@jail' => $jail->name)));
    else
        drupal_set_message(t('Jail @jail snapshot unsuccessful', array('@jail' => $jail->name)));

    $form_state['rebuild'] = true;

    drupal_goto("jailadmin/status");
}
