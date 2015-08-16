<?php

/**
 * Menu callbacks.
 */

/**
 * Submit actions for add2basket link.
 */
function geneshop_cart_add2basket($node, $type) {
  // Verification of the absence of this node in basket.
  if (_geneshop_cart_in_basket($node->nid)) {
    drupal_set_message(t('"@title" is already in your basket', array('@title' => $node->title)
      ), 'warning');
  }
  else {
    global $user;
    if ($user->uid == 0 && !drupal_session_started()) {
      // Must initialize sessions for anonymous users.
      drupal_session_start();
    }
    $item = array();
    $item['title'] = $node->title; // Tongta delete Qty
    if ($item) {
      $_SESSION['basket']['items'][$node->nid] = $item;
      drupal_set_message(t("'@title' added in your basket", array('@title' => $node->title)));
    }
  }
  if ($type == 'ajax') {
    $basket_block = geneshop_cart_block_content();
    $commands = array();
    $commands[] = ajax_command_replace(NULL, geneshop_cart_add2basket_link($node->nid));
    $commands[] = ajax_command_replace('#geneshop_cart-ajax-block-content', render($basket_block));
    $output = array('#type' => 'ajax', '#commands' => $commands);
    ajax_deliver($output);
  }
  else {
    drupal_goto();
  }
}

/**
 * Shared Ajax callback for basket page actions.
 */
function geneshop_cart_ajaxform_process() {
  if (!isset($_SESSION['basket']['items'])) {
    drupal_set_message(t('The form is invalid. Your basket is currently empty.'), 'error');
    return theme('status_messages');
  }
  $commands = array();
  $title = '';

  list($form, $form_state) = ajax_get_form();
  $element_name = $form_state['input']['_triggering_element_name'];

  switch ($element_name) {
    case 'clear':
      $form = drupal_get_form('_geneshop_cart_clear_confirm');
      $form['#action'] = 'basket/clear';
      $title = t('Do you really want to delete all items from the basket?');
      break;

    case 'checkout':
      $form = geneshop_cart_checkout(TRUE);
      $form['#action'] = 'basket/checkout';
      $title = t('Checkout');
      break;

    default:
      drupal_process_form($form['#form_id'], $form, $form_state);
  }

  if ($element_name == 'new_order' && !form_get_errors()) {
    $form = array(
      'type' => 'item',
      'value' => node_view($form_state['node']),
    );
    $title = t('Your order has been successfully created');
  }

  $commands[] = ajax_command_replace(NULL, render($form));

  if ($title) {
    $commands[] = ajax_command_html('#geneshop_cart-page-title *:first-child', $title);
  }

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * The form that displays the contents of basket and control buttons.
 */
function geneshop_cart_basket_form($form, &$form_state) {
  // Build Breadcrumbs.
  $breadcrumb = array();
  $breadcrumb[] = l(t('Home'), '');
  // Link to current URL.
  $breadcrumb[] = l(drupal_get_title(), current_path());
  // Set Breadcrumbs.
  drupal_set_breadcrumb($breadcrumb);

  // Check for empty session.
  if (!isset($_SESSION['basket']['items'])) {
    drupal_set_title(t('Your basket is empty'));
    $form['empty'] = array(
      '#type' => 'markup',
      '#markup' => t('There is nothing in your basket at the moment.'),
    );
  }
  else {
    $form['#id'] = drupal_html_id('geneshop_cart_basket_form');
    $form['#attributes'] = array(
      'class' => array('geneshop_cart_basket'),
    );
    $ajax_settings = array(
      'path' => 'geneshop_cart/ajax',
      'wrapper' => $form['#id'],
    );

    $form['clear'] = array(
      '#type' => 'submit',
      '#value' => t('Clear the basket'),
      '#name' => 'clear',
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('clear-button'),
      ),
    );

    $form['items'] = array(
      '#tree' => TRUE,
      '#theme' => 'geneshop_cart_items_form',
    );

    foreach ($_SESSION['basket']['items'] as $nid => $data) {
      $form['items'][$nid]['title'] = array(
        '#type' => 'link',
        '#title' => $data['title'],
        '#href' => 'node/' . $nid,
      );  // Tongta delete Qty and Price in table
//      $form['items'][$nid]['mRNA'] = array(
//        '#type' => 'checkbox',
//      );
//      $form['items'][$nid]['CDs'] = array(
//        '#type' => 'checkbox',
//      );
      $form['items'][$nid]['del'] = array(
        '#type' => 'checkbox',
      );
    }

    $form['actions'] = array(
      '#type' => 'actions',
      '#attributes' => array(
        'class' => array('geneshop_cart-form-actions'),
      ),
    );
    $form['actions']['update'] = array(
      '#type' => 'submit',
      '#submit' => array('_geneshop_cart_basket_update'),
      '#value' => t('Update'),
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('update-button'),
      ),
    );

    $form['actions']['checkout'] = array(
      '#type' => 'submit',
      '#name' => 'checkout',
      '#value' => t('Checkout'),
      '#ajax' => $ajax_settings,
      '#attributes' => array(
        'class' => array('checkout-button'),
      ),
    );
    $form['#validate'][] = 'geneshop_cart_check_empty_basket';
    $form['#attached'] = array(
      'css' => array(
        drupal_get_path('module', 'geneshop_cart') . '/theme/geneshop_cart.css',
      ),
      'js' => array(
        drupal_get_path('module', 'geneshop_cart') . '/theme/geneshop_cart.js',
      ),
    );
  }
  return $form;
}

/**
 * Basket page form qty element validate callback.
 */
//function _geneshop_cart_qty_element_validate($element, &$form_state, $form) {
//  if (!filter_var($element['#value'], FILTER_VALIDATE_INT) || $element['#value'] <= 0) {
//    form_error($element, t('Incorrect value of qty'));
//  }
//}

/**
 * Basket page form submit callback.
 */
function geneshop_cart_basket_form_submit($form, &$form_state) {
  switch ($form_state['triggering_element']['#name']) {
    case 'clear':
      $form_state['redirect'] = 'basket/clear';
      break;

    case 'checkout':
      $form_state['redirect'] = 'basket/checkout';
      break;
  }
}

/**
 * Handling data update form the basket page.
 */
function _geneshop_cart_basket_update($form, &$form_state) {
  $values = $form_state['values']['items'];
  $updated = FALSE;
  foreach ($values as $key => $val) {
    if ($val['del']) {
      drupal_set_message(t('"%title" was removed from your basket', array(
            '%title' => $_SESSION['basket']['items'][$key]['title'],
          )));
      unset($_SESSION['basket']['items'][$key]);
      if (isset($_SESSION['basket']['store'][$key])) {
        unset($_SESSION['basket']['store'][$key]);
      }
      $updated = TRUE;
    }
  }
  if ($updated) {
    $form_state['rebuild'] = TRUE;
    _geneshop_cart_update_total();
  }
}

/**
 * Updating total sum of basket items.
 */
function _geneshop_cart_update_total() {
  if (empty($_SESSION['basket']['items'])) {
    unset($_SESSION['basket']);
  }
}

/**
 * Calling the checkout form.
 */
function geneshop_cart_checkout($ajax = FALSE) {
  module_load_include('inc', 'node', 'node.pages');
  $add_form = node_add('geneshop_cart_order');
  return $ajax ? $add_form : drupal_render($add_form);
}

/**
 * Confirmation form when cleaning basket.
 */
function _geneshop_cart_clear_confirm($form, &$form_state) {
  if (isset($_SESSION['basket']['items'])) {
    return confirm_form($form,
      t('Do you really want to delete all items from the basket?'),
      'basket',
      t('This operation is irreversible'),
      t('Yes, clear'),
      t('Cancel')
    );
  }
}

/**
 * Confirmation form submit callback.
 */
function _geneshop_cart_clear_confirm_submit($form, &$form_state) {
  unset($_SESSION['basket']);
  drupal_set_message(t('Your basket is cleared'));
  drupal_goto('basket');
}

/**
 * Helper function to check the access code when referring to a specific order.
 */
function _geneshop_cart_get_my_order($node) {
  $code = isset($_GET['c']) && !empty($_GET['c']) ? check_plain($_GET['c']) : FALSE;
  $node_code = isset($node->order_code) ? $node->order_code : FALSE;
  if ($code && $node_code && ($code === $node_code)) {
    $_SESSION['order_codes'][] = $code;
    drupal_goto('node/' . $node->nid);
  }
  drupal_not_found();
}

/**
 * Menu callback; displays page containing recent orders of a given user.
 */
function geneshop_cart_my_orders_page($account) {
  drupal_set_title(t("@name's Order History", array('@name' => format_username($account))), PASS_THROUGH);

  $build = array();
  $query = db_select('node', 'n')->extend('PagerDefault');
  $nids = $query->fields('n', array('nid', 'sticky', 'created'))->condition('type', geneshop_cart_ORDER_NODETYPE)->condition('uid', $account->uid)->condition('status', 1)->orderBy('created', 'DESC')->limit(variable_get('default_nodes_main', 10))->addTag('node_access')->execute()->fetchCol();
  if (!empty($nids)) {
    $nodes = node_load_multiple($nids);
    $build += node_view_multiple($nodes);
    $build['pager'] = array(
      '#theme' => 'pager',
      '#weight' => 5,
    );
  }
  else {
    drupal_set_message(t('You have no orders for this account.'));
  }

  return $build;
}
