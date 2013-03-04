<?php
/**
 * @file
 * Commerce payment callbacks
 */

/**
 * Returns the default settings for the PayPal WPS payment method.
 */
function commerce_jcc_default_settings() {
  //$default_currency = variable_get('commerce_default_currency', 'USD');

  return array(
    'Server' => 'sandbox',
    'Version' => '1.0.0',
    'MerID' => '',
    'AcqID' => '',
    'MerRespURL' => url('commerce_jcc/process', array('absolute' => TRUE, 'https' => TRUE)),
    'PurchaseCurrency' => 978,
    'PurchaseCurrencyExponent' => 2,
    'CaptureFlag' => 'A',
    'Password' => '',
    'SignatureMethod' => 'SHA1',
  );
}

/**
 * Payment method callback: settings form.
 */
function commerce_jcc_settings_form($settings = array()) {
  $form = array();

  // Merge default settings into the stored settings array.
  $settings = (array) $settings + commerce_jcc_default_settings();

  $form['Server'] = array(
    '#type' => 'radios',
    '#title' => t('PayPal server'),
    '#options' => array(
      'sandbox' => ('Sandbox - use for testing.'),
      'live' => ('Live - use for processing real transactions'),
    ),
    '#default_value' => $settings['Server'],
  );
  $form['Version'] = array(
    '#type' => 'textfield',
    '#title' => t("API Version"),
    '#default_value' => $settings['Version'],
    '#disabled' => TRUE,
  );
  $form['MerID'] = array(
    '#type' => 'textfield',
    '#title' => t("Merchant ID"),
    '#default_value' => $settings['MerID'],
    '#element_validate' => array('element_validate_integer_positive'),
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['AcqID'] = array(
    '#type' => 'textfield',
    '#title' => t("Acquirer ID"),
    '#default_value' => $settings['AcqID'],
    '#element_validate' => array('element_validate_integer_positive'),
    '#maxlength' => 6,
    '#required' => TRUE,
  );
  $form['MerRespURL'] = array(
    '#type' => 'textfield',
    '#title' => t("Responce URL"),
    '#default_value' => $settings['MerRespURL'],
    '#descriptions' => "The SSL protected URL of the merchant to which JCC will send the response, i.e. a script on the merchant’s server that will process the response from JCC ",
    '#element_validate' => array('_commerce_jcc_validate_responce_url'),
    '#disabled' => TRUE,
  );
  // todo: change to select list with all possible currency value
  $form['PurchaseCurrency'] = array(
    '#type' => 'textfield',
    '#title' => t("Purchase currency"),
    '#default_value' => $settings['PurchaseCurrency'],
    '#element_validate' => array('element_validate_integer_positive'),
    '#maxlength' => 3,
    '#desctiptions' => "The ISO code for the currency of the transaction; the value specified needs to be the ISO code corresponding to one of the currencies supported by JCC’s payment gateway – see relevant Appendix for details.",
  );
  $form['PurchaseCurrencyExponent'] = array(
    '#type' => 'textfield',
    '#title' => t("Purchase currency exponent"),
    '#default_value' => $settings['PurchaseCurrencyExponent'],
    '#element_validate' => array('element_validate_integer_positive'),
    '#maxlength' => 1,
    '#description' => "The number of decimal digits of the purchase currency; can be 0, 2 or 3.",
  );
  $form['CaptureFlag'] = array(
    '#type' => 'select',
    '#title' => t("Capture flag"),
    '#default_value' => $settings['CaptureFlag'],
    '#options' => drupal_map_assoc(array('A', 'M')),
    '#description' => "Indicates whether to only authorize (block) the amount in cardholder’s account or to authorize and automatically capture. Possible values: A: automatically capture, M: do not automatically capture; will capture manually at a later stage. In case, M is specified, the blocked amount can be captured later, before expiration of the authorization, through the Merchant Backoffice System, through a Batch or by using web services.",
  );
  $form['Password'] = array(
    '#type' => 'textfield',
    '#title' => t("Password"),
    '#default_value' => $settings['Password'],
  );
  $form['SignatureMethod'] = array(
    '#type' => 'select',
    '#title' => t("Signature method"),
    '#default_value' => $settings['SignatureMethod'],
    '#options' => drupal_map_assoc(array('SHA1', 'MD5')),
    '#description' => "The algorithm used to create the signature; SHA1 or MD5 (SHA1 is preferred).",
    '#disabled' => TRUE,
  );

  return $form;
}

/**
 * Redirects the user to the hosted payment page.
 */
function commerce_jcc_redirect_form($form, &$form_state, $order, $payment_method) {
  // Return an error if MerID is not set.
  if (empty($payment_method['settings']['MerID'])) {
    drupal_set_message(t('JCC Merchant ID has not been specified.'), 'error');
    return array();
  }
  // Return an error if AcqID is not set.
  if (empty($payment_method['settings']['MerID'])) {
    drupal_set_message(t('JCC Acquirer ID has not been specified.'), 'error');
    return array();
  }
  // Return an error if password is not set.
  if (empty($payment_method['settings']['Password'])) {
    drupal_set_message(t('JCC Password needs to be set.'), 'error');
    return array();
  }

  return commerce_jcc_order_form($form, $form_state, $order, $payment_method['settings']);
}

/**
 * Defines the order form.
 *
 * This form is displayed directly before redirecting to Moneybookers hosted
 * payment page.
 */
function commerce_jcc_order_form($form, &$form_state, $order, $settings) {
  $wrapper = entity_metadata_wrapper('commerce_order', $order);
  $amount = $wrapper->commerce_order_total->amount->value();

  $formattedPurchaseAmt = commerce_jcc_format_amount($amount, $settings['PurchaseCurrencyExponent']);
  //Form the plaintext string to encrypt by concatenating Password, Merchant ID, Acquirer ID, Order ID, Formatter Purchase Amount and Currency
  //This will give 1234abcd | 0011223344 | 402971 | TestOrder12345 | 000000002050 | 978 (spaces and | introduced here for clarity)
  $toEncrypt = $settings['Password'].$settings['MerID'].$settings['AcqID'].$order->order_id.$formattedPurchaseAmt.$settings['PurchaseCurrency'];
  //Produce the hash using SHA1
  //This will give b14dcc7842a53f1ec7a621e77c106dfbe8283779
  $sha1Signature = sha1($toEncrypt);
  //Encode the signature using Base64 before transmitting to JCC //This will give sU3MeEKlPx7HpiHnfBBt++goN3k=
  $base64Sha1Signature = base64_encode(pack("H*",$sha1Signature));

  // Build the data array that will be translated into hidden form values.
  $data = array(
    'Version' => '1.0.0',
    'MerID' => $settings['MerID'],
    'AcqID' => $settings['AcqID'],
    'MerRespURL' => $settings['MerRespURL'],
    'PurchaseAmt' => $formattedPurchaseAmt,
    'PurchaseCurrency' => $settings['PurchaseCurrency'],
    'PurchaseCurrencyExponent' => $settings['PurchaseCurrencyExponent'],
    'OrderID' => $order->order_id,
    'CaptureFlag' => $settings['CaptureFlag'],
    'Signature' => $base64Sha1Signature,
    'SignatureMethod' => $settings['SignatureMethod'],
  );

  $form['#action'] = commerce_jcc_server_url($settings['Server']);

  foreach ($data as $name => $value)
    if (!empty($value))
      $form[$name] = array('#type' => 'hidden', '#value' => $value);

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'submit',
  );

  return $form;
}

/**
 * Returns the URL to the specified PayPal WPS server.
 *
 * @param $server
 *   Either sandbox or live indicating which server to get the URL for.
 *
 * @return string
 *   The URL to use to submit requests to the PayPal WPS server.
 */
function commerce_jcc_server_url($server) {
  switch ($server) {
    case 'sandbox':
      return 'https://tjccpg.jccsecure.com/EcomPayment/RedirectAuthLink';
    case 'live':
      return 'https://jccpg.jccsecure.com/EcomPayment/RedirectAuthLink';
  }
}

/**
 * Formate amount
 *
 * @param float $amount
 * @param int $exponent
 *
 * @return int
 */
function commerce_jcc_format_amount($amount) {
  return str_pad($amount, 12, "0", STR_PAD_LEFT);
}