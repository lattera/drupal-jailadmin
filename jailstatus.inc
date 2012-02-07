<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Mount.php');
require_once('classes/Service.php');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_status($form, &$form_state) {
    $jails = Jail::LoadAll();

    $header = array('Jail Name', 'Status', 'Network Status');
    $rows = array();
    foreach ($jails as $jail) {
        if (user_access('config ' . $jail->name) || user_access('view ' . $jail->name . ' config'))
            $name = l(t($jail->name), 'jailadmin/' . $jail->name . '/config');
        else
            $name = t('@name', array('@name' => $jail->name));

        $rows[] = array($name, $jail->IsOnlineString(), $jail->NetworkStatus());
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
