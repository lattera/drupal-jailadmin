<?php

function jailadmin_admin_settings($form, &$form_state) {
    $form['jailadmin'] = array(
        '#type' => 'markup',
        '#prefix' => '<div>',
        '#markup' => t('Under construction!'),
        '#suffix' => '</div>',
    );

    return $form;
}
