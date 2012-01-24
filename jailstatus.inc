<?php

require_once('jailadmin.helpers.inc');
require_once('classes/Network.php');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_status($form, &$form_state) {
    $jails = Jail::LoadAll();

    $header = array('Jail Name', 'Status');
    $rows = array();
    foreach ($jails as $jail) {
        $name = l(t($jail->name), 'jailadmin/' . $jail->name . '/config');
        $rows[] = array($name, $jail->IsOnlineString());
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
