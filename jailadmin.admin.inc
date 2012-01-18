<?php

require_once('jailadmin.helpers.inc');
require_once('classes/NetworkDevice.php');
require_once('classes/Jail.php');

function jailadmin_admin_settings($form, &$form_state) {
    $jails = get_all_jails_for_select();

    if (count($jails)) {
        $form['jails'] = array(
            '#type' => 'fieldset',
            '#title' => t('Configured Jails'),
            '#collapsible' => TRUE,
            '#collapsed' => FALSE,
        );

        $form['jails']['existing'] = array(
            '#type' => 'select',
            '#title' => t('Existing Jails'),
            '#multiple' => TRUE,
            '#description' => t('Select one or more to delete'),
            '#options' => $jails,
        );
    }

    $form['new_jail'] = array(
        '#type' => 'fieldset',
        '#title' => t('Add New Jail'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
    );

    $form['new_jail']['new_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Name'),
        '#size' => 30,
    );

    $form['new_jail']['new_path'] = array(
        '#type' => 'textfield',
        '#title' => t('Path'),
        '#size' => 30,
    );

    $form['new_jail']['new_dataset'] = array(
        '#type' => 'textfield',
        '#title' => t('Dataset'),
        '#size' => 30,
    );

    $form['new_jail']['new_route'] = array(
        '#type' => 'textfield',
        '#title' => t('Default Route'),
        '#size' => 30,
    );

    $form['new_jail']['new_template'] = array(
        '#type' => 'textfield',
        '#title' => t('Template Jail dataset@snapshot'),
        '#size' => 30,
        '#description' => t('Only if creating from a template'),
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Submit'),
    );

    return $form;
}

function jailadmin_admin_settings_submit($form, &$form_state) {
    if (isset($form_state['values']['existing'])) {
        foreach (array_filter($form_state['values']['existing']) as $name) {
            $j = Jail::Load($name);
            $j->Delete();
        }
    }

    if (isset($form_state['values']['new_name']) && strlen($form_state['values']['new_name'])) {
        $name = $form_state['values']['new_name'];
        $path = $form_state['values']['new_path'];
        $dataset = $form_state['values']['new_dataset'];
        $route = $form_state['values']['new_route'];

        $j = new Jail;
        $j->name = $name;
        $j->path = $path;
        $j->dataset = $dataset;
        $j->route = $route;

        $j->Create();
    }

    $form_state['rebuild'] = TRUE;
}
