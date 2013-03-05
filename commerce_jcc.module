<?php
/**
 * @file
 * Implements JCC payment services for use with Drupal Commerce.
 */

/**
 * Implements of hook_menu()
 *
 * @see hook_menu()
 */
function commerce_jcc_menu() {
  $items['commerce_jcc/process'] = array(
    'page callback' => 'commerce_jcc_result_page',
    'access callback' => TRUE,
    'type' => MENU_CALLBACK,
  );
  return $items;
}

/**
 * Implements of hook_commerce_payment_method_info()
 *
 * @see hook_commerce_payment_method_info()
 *
 * @return
 *   An array of payment methods, using the format defined above.
 */
function commerce_jcc_commerce_payment_method_info() {
  $payment_methods['jcc'] = array(
    'base' => 'commerce_jcc',
    'title' => t('JCC'),
    'short_title' => t('JCC'),
    'description' => t('JCC Credit Card Payment'),
    'terminal' => FALSE,
    'offsite' => TRUE,
    'offsite_autoredirect' => TRUE,
    'file' => 'commerce_jcc.commerce.inc'
  );
  return $payment_methods;
}

// todo: move to another file
function commerce_jcc_result_page() {
  if (isset($_POST['OrderID']) && $order = commerce_order_load((int) $_POST['OrderID'])) {
    $wrapper = entity_metadata_wrapper('commerce_order', $order);
    $transaction = commerce_payment_transaction_new('jcc', $order->order_id);

    // todo: status & message dont set =(
    if (!isset($_POST['ResponseCode']) && !isset($_POST['ReasonCode'])) {
      $transaction->status = COMMERCE_PAYMENT_STATUS_FAILURE;
      drupal_set_message('No POST');
      $transaction->message = t('No POST data received');
    } elseif ($_POST['ResponseCode'] == 1 && $_POST['ReasonCode'] == 1) {
      $transaction->status = 'completed';
      drupal_set_message('Completed');
      $transaction->message = t('Payment succeeded with message: %message', array('%message' => $_POST['ReasonCodeDesc']));
    } else {
      // todo: create reason codes list and return them from pdf Appendix B
      $transaction->status = 'canceled';
      drupal_set_message('Canceled');
      $transaction->message = t('Payment failed with code %code and message: %message', array('%code' => $_POST['ReasonCode'], '%message' => $_POST['ReasonCodeDesc']));
    }

    $amount = $wrapper->commerce_order_total->amount->value();
    $currency = $wrapper->commerce_order_total->currency_code->value();
    $transaction->amount = $amount;
    $transaction->currency_code = $currency;

    commerce_payment_transaction_save($transaction);

    $output = '';
    if (isset($_POST['ResponseCode'])) {
      dpm($_POST);
      $output .= t('Payment failed with code %code and message: %message', array('%code' => $_POST['ReasonCode'], '%message' => $_POST['ReasonCodeDesc']));
    }
    return $output;
  }
  drupal_not_found();
  return NULL;
}

function _commerce_jcc_validate_responce_url($element, &$form_state) {
  $value = $element['#value'];
  if ($value !== '' && valid_url($element['#value'], TRUE) !== TRUE) {
    form_error($element, t('%name is not a valid url', array('%name' => $element['#title'])));
  }
}