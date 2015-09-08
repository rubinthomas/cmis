<?php
/**
 * Config sample:
 * Run CMIS Sync Help <a href='admin/help#cmis_sync'>admin/help#cmis_sync</a>
 *
 *
 *
 */
/**
 * @file
 * Admin functions for cmis_sync module.
 */

/**
 * Menu callback and form builder for admin/settings/cmis_sync.
 */
function cmis_sync_admin_settings() {
  $page = array(
      'title' => t('Settings.php \'cmis_sync\' Configuration Value Check'),
      'description' => t('Global CMIS Document \'cmis_sync\' Configuration check'),
    );
  return cmis_sync_setting_validate();
}

/**
 * Validate that the cmis_sync_setting_validate exists.
 */
function cmis_sync_setting_validate() {
  global $conf;
  module_load_include('api.inc', 'cmis');
  module_load_include('cmis.inc', 'cmis_sync');
  $min_global_conf = array('enabled','cmis_type','content_field','cmis_folderId','fields','subfolders','cmis_sync_cron','cmis_sync_cron_enabled');
  $ret = "<div class=\"messages status\">\n";
  if(array_key_exists('cmis_sync_map',$conf) && is_array($conf['cmis_sync_map'])) {
    if (!empty($conf['cmis_sync_map'])) {
      foreach($conf['cmis_sync_map'] as $node_type => $v) {
        foreach($min_global_conf as $m) {
          if(!array_key_exists($m,$v))
            drupal_set_message(t('Missing Global Configuration Key '.$m.' in settings.php'), 'error');
        }
        variable_set('cmis_sync_cron_enabled_'.$node_type, $v['cmis_sync_cron_enabled']);
        $ret .= t("<h5>Global Configuration Settings ['".ucfirst($node_type)."'] : </h5>\n<pre>".print_r($v,1)."</pre>\n");
      }
    }
    else
      drupal_set_message(t('Missing Global Configuration [\'cmis_sync_map\']Array \'node_type\' in settings.php'), 'error',FALSE);
  }
  else
    drupal_set_message(t('Missing Global Configuration [\'cmis_sync_map\']Array in settings.php'), 'error',FALSE);

  return $ret."</div>\n";
}

/**
 * CMIS
 *
 * @return $form
 */
function cmis_sync_admin_form($form, &$form_state) {
  $query = db_select('cmis_sync_node', 'c');
  $query->condition('c.id', arg(4), '=')
    ->fields('c', array('id','nid', 'cmis_repositoryId','cmis_objectId','cmis_versionLabel','changed_timestamp'));
  $results = $query->execute();
  $form = array();
  if(!empty($results)) {
    $form['cmis_sync'] = array();
    foreach ($results as $row) {
      $form['cmis_sync']['id'] = array(
        '#type' => 'hidden',
        '#value' => $row->id,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync']['nid'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Node',
        '#value' => $row->nid,
        '#description' => get_node_title($row->nid),
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync']['cmis_objectId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS ObjectID',
        '#value' => $row->cmis_objectId,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync']['cmis_repositoryId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS RepositoryID',
        '#value' => $row->cmis_repositoryId,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync']['cmis_versionLabel'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Version',
        '#value' => $row->cmis_versionLabel,
        '#no_cache' => TRUE,
      );
    }
    $form['cmis_sync']['actions'] = array('#type' => 'actions');
    $form['cmis_sync']['actions']['submit'] = array(
      '#type' => 'submit',
      '#id' => 'cmis-sync-submit',
      '#value' => t('Save CMIS Document Sync'),
      '#submit' => array('cmis_sync_admin_form_submit'),
    );
    $form['cmis_sync']['actions']['cancel'] = array(
      '#type' => 'link',
      '#button_type' => 'button',
      '#title' => t('Cancel'),
      '#href' => '/admin/cmis/cmis_sync',
    );
  }
  return $form;
}

/**
 * Form validation
 */
function cmis_sync_admin_form_validate($form, &$form_state){
  if (empty($form_state['values']['id'])){
    form_set_error('cmis_sync', 'Error Please check CMIS Sync ID');
  }
}

/**
 * Form submission
 */
function cmis_sync_admin_form_submit(&$form, &$form_state){
  $num_updated = db_update('cmis_sync_node')
    ->fields(array(
      'cmis_repositoryId' => trim($form_state['values']['cmis_repositoryId']),
      'cmis_objectId' => trim($form_state['values']['cmis_objectId']),
      'cmis_versionLabel' => trim($form_state['values']['cmis_versionLabel']),
      'changed_timestamp' => REQUEST_TIME
    ))
    ->condition('id', intval($form_state['values']['id']), '=')
    ->execute();
  drupal_set_message('CMIS Node '.$form_state['values']['nid'].' - ( '.get_node_title($form_state['values']['nid']).' ) has been updated');
  drupal_goto('/admin/cmis/cmis_sync');
}
/**
 * CMIS
 *
 * @return $form
 */
function cmis_sync_errors_form($form, &$form_state) {
  $form = array();
  $ret = "<p>Check all Errored Nodes to see if they are available in Drupal but not in the CMIS Repository</p>"."\n";
  $ret .= "<p>".date('Y-m-d H:i:s', $_SERVER['REQUEST_TIME'])."</p>"."\n";

  $form['cmis_sync'] = array(
    '#markup' => $ret,
    '#title' => t('CMIS Sync Errored Cron')
  );
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#markup' => $ret,
    '#title' => t('Filter option')
  );
  $form['filter']['filter_objectId'] = array(
    '#type' => 'textfield',
    '#title' => t('CMIS ObjectID'),
    '#size' => 40,
  );
  $form['filter']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
  );
  $form['filter']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#markup' => l(t('Reset'), '/admin/cmis/cmis_sync/cmis/errors'),
  );

  $header = array(
    array('data' => 'CMIS ID', 'field' => 'id', 'sort' => 'ASC'),
    array('data' => 'Drupal Node ID', 'field' => 'nid', 'sort' => 'ASC'),
    array('data' => 'CMIS Repository', 'field' => 'cmis_repositoryId', 'sort' => 'ASC'),
    array('data' => 'CMIS ObjectID', 'field' => 'cmis_objectId', 'sort' => 'ASC'),
    array('data' => 'CMIS Version', 'field' => 'cmis_versionLabel', 'sort' => 'ASC'),
    array('data' => 'Timestamp', 'field' => 'changed_timestamp', 'sort' => 'ASC'),
  );

  $query = db_select('cmis_sync_node', 'p')->extend('PagerDefault')->extend('TableSort');
  $query->condition('nid', db_select('node','n')->fields('n', array('nid')), 'NOT IN');

  if (isset($form_state['values']['filter_objectId'])) {
    $query->condition('cmis_objectId', '%' . db_like($form_state['values']['filter_objectId']) . '%', 'LIKE');
  }
  if (isset($form_state['values']['filter_cmisid'])) {
    $query->condition('id', intval($form_state['values']['filter_cmisid']), '=');
  }
  if (isset($form_state['values']['filter_nid'])) {
    $query->condition('nid', intval($form_state['values']['filter_nid']), '=');
  }


  $results = $query
    ->fields('p', array('id','nid', 'cmis_repositoryId','cmis_objectId','cmis_versionLabel','changed_timestamp'))
    ->orderByHeader($header)
    ->orderBy('id', 'DESC')
    ->limit(CMIS_SYNC_IMPORT_BATCH_LIMIT)
    ->execute();

  $rows = array();
  if(!empty($results)) {
    foreach ($results as $row) {
      $rows[] = array("<a href=\"/admin/cmis/cmis_sync/cmis/errors/".$row->id."/edit\">".$row->id."</a>",
            $row->nid,
            $row->cmis_repositoryId,
            $row->cmis_objectId,
            $row->cmis_versionLabel,
            date("j F Y", $row->changed_timestamp)
      );
    }
  }

  $form['table'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#empty' => t('There is no data')
  );
  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}

function cmis_sync_errors_form_submit($form, &$form_state) {
  form_state_values_clean($form_state);
  $form_state['rebuild'] = TRUE;
  if(isset($form_state['values']['reset']))
    unset($form_state);
}

function cmis_sync_errors_delete_form($form, &$form_state) {
  $query = db_select('cmis_sync_node', 'c');
  $query->condition('c.id', arg(5), '=')
    ->fields('c', array('id','nid', 'cmis_repositoryId','cmis_objectId','cmis_versionLabel','changed_timestamp'));

  $results = $query->execute();
  $form = array();
  if(!empty($results)) {
    $form['cmis_sync_errors'] = array();
    foreach ($results as $row) {
      $form['cmis_sync_errors']['id'] = array(
        '#type' => 'hidden',
        '#value' => $row->id,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync_errors']['nid'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Node',
        '#value' => $row->nid,
        '#description' => get_node_title($row->nid),
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync_errors']['cmis_objectId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS ObjectID',
        '#value' => $row->cmis_objectId,
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync_errors']['cmis_repositoryId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS RepositoryID',
        '#value' => $row->cmis_repositoryId,
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
      $form['cmis_sync_errors']['cmis_versionLabel'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Version',
        '#value' => $row->cmis_versionLabel,
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
    }
    $form['cmis_sync_errors']['actions'] = array('#type' => 'actions');
    $form['cmis_sync_errors']['actions']['submit'] = array(
      '#type' => 'submit',
      '#id' => 'cmis-sync-error-submit',
      '#value' => t('Delete Errored CMIS Sync Node'),
      '#submit' => array('cmis_sync_errors_delete_form_submit'),
    );
    $form['cmis_sync_errors']['actions']['cancel'] = array(
      '#type' => 'link',
      '#button_type' => 'button',
      '#title' => t('Cancel'),
      '#href' => '/admin/cmis/cmis_sync/cmis/errors',
    );
  }
  return $form;
}

/**
 * Form validation
 */
function cmis_sync_errors_delete_form_validate($form, &$form_state){
  if (empty($form_state['values']['id'])){
    form_set_error('cmis_sync_errors', 'Error Please check CMIS Sync ID');
  }
}

/**
 * Form submission
 */
function cmis_sync_errors_delete_form_submit(&$form, &$form_state){
  $num_deleted = db_delete('cmis_sync_node')
    ->condition('id', intval($form_state['values']['id']))
    ->execute();
  drupal_set_message('CMIS Errored Node '.$form_state['values']['nid'].' has been deleted');
  drupal_goto('/admin/cmis/cmis_sync/cmis/errors');
}

