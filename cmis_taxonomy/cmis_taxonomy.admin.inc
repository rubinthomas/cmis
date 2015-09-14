<?php
/**
 * Module code to import of CMIS Tag/Category data into 'taxonomy'.
 *
 *
 *
 *
 */
/**
 * @file
 * Admin functions for cmis_taxonomy module.
 */

/**
 * Menu callback and form builder for admin/cmis/cmis_taxonomy/vocabulary.
 */
function cmis_taxonomy_vocabulary(){
  $header = array(
    array('data' => 'Vocabulary', 'field' => 'cmis_vname', 'sort' => 'ASC'),
    array('data' => 'CMIS Repository', 'field' => 'cmis_repositoryId', 'sort' => 'ASC'),
    array('data' => 'CMIS Cat ObjectID', 'field' => 'cmis_objectId', 'sort' => 'ASC'),
    array('data' => 'CMIS Synced', 'field' => 'cmis_exclude', 'sort' => 'ASC'),
    array('data' => 'Timestamp', 'field' => 'timestamp', 'sort' => 'ASC'),
  );
  $query = db_select('cmis_vocab', 'p')->extend('PagerDefault')->extend('TableSort');
  $query->fields('p', array('cmis_vid', 'vid', 'cmis_vname', 'cmis_repositoryId', 'cmis_objectId', 'cmis_exclude','timestamp'))
    ->orderByHeader($header)
    ->orderBy('timestamp', 'DESC')
    ->limit(100);
  $results = $query->execute();
  if(!empty($results)) {
    $rows = array();
    $options = array(0 => 'Included', 1 => 'Excluded');
    foreach ($results as $row) {
      $rows[] = array("<a href=\"/admin/cmis/cmis_taxonomy/vocabulary/edit/".$row->cmis_vid."\">".$row->cmis_vname."</a>",
            $row->cmis_repositoryId,
            $row->cmis_objectId,
            $options[$row->cmis_exclude],
            date("j F Y", $row->timestamp)
      );
    }
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows ));
  $output .= theme('pager');

  return $output;
}

function cmis_taxonomy_drupal_vocabulary_list() {
  $query = db_select('taxonomy_vocabulary', 'tv');
  $query->fields('tv', array('vid', 'name', 'machine_name'));
  $vocabulary = $query->execute();
  if(!empty($vocabulary)) {
    foreach ($vocabulary as $voc) {
      $vocab[$voc->vid] = $voc->name;
    }
    return $vocab;
  }
  return NULL;
}

function cmis_taxonomy_vocabulary_admin_form($form, &$form_state) {
  $vocab = cmis_taxonomy_drupal_vocabulary_list();
  if(empty($vocab)) {
    drupal_set_message('No Vocabulary existing. Please create them in <a href="/admin/structure/taxonomy/add">Taxonomy</a>.');
    exit();
  }
  $options = array(0 => 'Included', 1 => 'Excluded');
  $query = db_select('cmis_vocab', 'c');
  $query->condition('c.cmis_vid', arg(5), '=')
    ->fields('c', array('cmis_vid', 'vid', 'cmis_vname', 'cmis_repositoryId', 'cmis_objectId', 'cmis_exclude', 'timestamp'));
  $results = $query->execute();
  $form['cmis_vocab'] = array();
  if(!empty($results) && ($results->rowCount() > 0)) {
    foreach ($results as $row) {
      $form['cmis_vocab']['cmis_vid'] = array(
        '#type' => 'hidden',
        '#value' => $row->cmis_vid,
        '#required' => TRUE,
      );
      $form['cmis_vocab']['cmis_vname'] = array(
        '#type' => 'hidden',
        '#title' => 'CMIS Vocabulary Name',
        '#value' => $row->cmis_vname,
        '#required' => TRUE,
      );
      $form['cmis_vocab']['vid'] = array(
        '#type' => 'select',
        '#title' => 'Choose a Drupal Vocabulary to Map to CMIS Vocabulary',
        '#options' => $vocab,
        '#default_value' => $row->vid,
        '#required' => TRUE,
      );
      $form['cmis_vocab']['cmis_objectId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS ObjectID',
        '#value' => $row->cmis_objectId,
        '#required' => TRUE,
      );
      $form['cmis_vocab']['cmis_repositoryId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS RepositoryID',
        '#value' => $row->cmis_repositoryId,
        '#required' => TRUE,
      );
      $form['cmis_vocab']['cmis_exclude'] = array(
        '#type' => 'select',
        '#title' => 'CMIS Synced',
        '#options' => $options,
        '#default_value' => $row->cmis_exclude,
        '#required' => TRUE,
      );
    }
  } else {
    $form['cmis_vocab']['vid'] = array(
      '#type' => 'select',
      '#title' => 'Choose a Drupal Vocabulary to Map to CMIS Vocabulary',
      '#options' => $vocab,
      '#default_value' => 0,
      '#required' => TRUE,
    );
    $form['cmis_vocab']['cmis_objectId'] = array(
      '#type' => 'textfield',
      '#title' => 'CMIS ObjectID',
      '#required' => TRUE,
    );
    $form['cmis_vocab']['cmis_repositoryId'] = array(
      '#type' => 'textfield',
      '#title' => 'CMIS RepositoryID',
      '#required' => TRUE,
    );
    $form['cmis_vocab']['cmis_exclude'] = array(
      '#type' => 'select',
      '#title' => 'CMIS Synced',
      '#options' => $options,
      '#default_value' => 0,
      '#required' => TRUE,
    );
  }
  $form['cmis_vocab']['actions'] = array('#type' => 'actions');
  $form['cmis_vocab']['actions']['submit'] = array(
    '#type' => 'submit',
    '#id' => 'cmis-taxonomy-vocabulary-submit',
    '#value' => t('Save CMIS Taxonomy Vocabulary Sync'),
    '#submit' => array('cmis_taxonomy_vocabulary_admin_form_submit'),
  );
  $form['cmis_vocab']['actions']['cancel'] = array(
    '#type' => 'link',
    '#button_type' => 'button',
    '#title' => t('Cancel'),
    '#href' => '/admin/cmis/cmis_taxonomy/vocabulary',
  );
  return $form;
}
/**
 * Form validation
 */
function cmis_taxonomy_vocabulary_admin_form_validate($form, &$form_state){
  if (empty($form_state['values']['vid'])){
    form_set_error('cmis_vocab', 'Error Please check Vocabulary ID');
  }
}

/**
 * Form submission
 */
function cmis_taxonomy_vocabulary_admin_form_submit(&$form, &$form_state){
  $vocab = cmis_taxonomy_drupal_vocabulary_list();
  if(empty($vocab))
    drupal_set_message('No Vocabulary existing. Please create them in <a href="/admin/structure/taxonomy/add">Taxonomy</a>.');
  else {
    $vid = intval($form_state['values']['vid']);
    if(!empty($form_state['values']['cmis_vid'])) {
      $num_updated = db_update('cmis_vocab')
        ->fields(array(
          'vid' => $vid,
          'cmis_vname' => trim(@$vocab[$vid]),
          'cmis_repositoryId' => trim($form_state['values']['cmis_repositoryId']),
          'cmis_objectId' => trim($form_state['values']['cmis_objectId']),
          'cmis_exclude' => intval($form_state['values']['cmis_exclude']),
          'timestamp' => REQUEST_TIME
        ))
        ->condition('cmis_vid', intval($form_state['values']['cmis_vid']), '=')
        ->execute();
        drupal_set_message('Vocabulary '.$vocab[$vid].' has been updated');
    }
    else {
      $query = db_select('cmis_vocab', 'c');
      $query->condition('c.vid', $vid, '=')
        ->fields('c', array('cmis_vid', 'vid', 'cmis_vname'));
      $exists = $query->execute();
      if(!empty($exists) && ($exists->rowCount() <= 0)) {
        $num_inserted = db_insert('cmis_vocab')
          ->fields(array('cmis_repositoryId', 'cmis_objectId', 'cmis_exclude', 'timestamp'))
          ->fields(array(
            'vid' => $vid,
            'cmis_vname' => trim(@$vocab[$vid]),
            'cmis_repositoryId' => trim($form_state['values']['cmis_repositoryId']),
            'cmis_objectId' => trim($form_state['values']['cmis_objectId']),
            'cmis_exclude' => intval($form_state['values']['cmis_exclude']),
            'timestamp' => REQUEST_TIME
          ))
          ->execute();
        drupal_set_message('Vocabulary "'.$vocab[$vid].'" has been Added');
      }
      else
        drupal_set_message('Vocabulary "'.$vocab[$vid].'" Exists. Please Edit Instead', 'error',FALSE);
    }
  }
  drupal_goto('/admin/cmis/cmis_taxonomy/vocabulary');
}

/**
 * Menu callback and form builder for admin/settings/cmis_taxonomy.
 */
function cmis_taxonomy_admin_settings() {
  $page = array(
    'title' => t('Settings.php \'cmis_taxonomy\' Configuration Value Check'),
    'description' => t('Global CMIS Taxonomy \'cmis_taxonomy\' Configuration check'),
  );
  return cmis_taxonomy_setting_validate();
}

/**
 * Validate that the cmis_taxonomy_setting_validate exists.
 */
function cmis_taxonomy_setting_validate() {
  global $conf;
  module_load_include('api.inc', 'cmis');
  module_load_include('cmis.inc', 'cmis_taxonomy');

  $ret = "<div class=\"messages status\">\n";
  if(array_key_exists('cmis_taxonomy',$conf) && array_key_exists('taxonomy',$conf['cmis_taxonomy']))  {
    if (!empty($conf['cmis_taxonomy']['taxonomy'])) {
      foreach($conf['cmis_taxonomy']['taxonomy'] as $taxonomy_type => $v) {
        $min_global_conf = array('enabled','cmis_repositoryId','cmis_type','cmis_tagorcat','content_field','cmis_folderId','cmis_folderPath');
        foreach($min_global_conf as $m) {
          if(!array_key_exists($m,$v))
            drupal_set_message(t('Missing Global Configuration Key '.$m.' in settings.php'), 'error');
        }
        $ret .= t("<h5>Global Configuration Settings ['".ucfirst($taxonomy_type)."'] : </h5>\n<pre>".print_r($v,1)."</pre>\n");
      }
    }
    else
      drupal_set_message(t('Missing Global Configuration [\'taxonomy\']Array \'cmis_taxonomy\' in settings.php'), 'error',FALSE);
  }
  else
    drupal_set_message(t('Missing Global Configuration [\'cmis_taxonomy\']Array in settings.php'), 'error',FALSE);
  return $ret."</div>\n";
}

function cmis_taxonomy_admin_form($form, &$form_state) {
  global $user;
  $query = db_select('cmis_taxonomy', 't');
  $query->condition('t.cmistxid', arg(4), '=')
    ->fields('t', array('cmistxid','vid','tid','cmis_name','cmis_drupal_name','cmis_repositoryId','cmis_objectId','cmis_tagorcat','cmis_lastModifiedBy','cmis_lastModificationDate','timestamp'));
  $results = $query->execute();
  $form = array();
  if(!empty($results)) {
    $form['cmis_taxonomy'] = array();
    foreach ($results as $row) {
      $form['cmis_taxonomy']['cmistxid'] = array(
        '#type' => 'hidden',
        '#value' => $row->cmistxid,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['vid'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Vocabulary ID',
        '#value' => $row->vid,
        '#disabled' => TRUE,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['tid'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Term ID',
        '#value' => $row->tid,
        '#disabled' => TRUE,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_name'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Term Name',
        '#value' => $row->cmis_name,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_drupal_name'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Drupal Term Name',
        '#value' => $row->cmis_drupal_name,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_repositoryId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS RepositoryID',
        '#value' => $row->cmis_repositoryId,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_objectId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS ObjectID',
        '#value' => $row->cmis_objectId,
        '#required' => TRUE,
      );
      $options = array('tag' => 'tag', 'category' => 'category');
      $form['cmis_taxonomy']['cmis_tagorcat'] = array(
        '#type' => 'select',
        '#title' => 'CMIS Tag/Category',
        '#options' => $options,
        '#default_value' => $row->cmis_tagorcat,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_lastModifiedBy'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Last Modified By',
        '#value' => $row->cmis_lastModifiedBy,
        '#required' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_lastModificationDate'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Last Modification Date',
        '#value' => $row->cmis_lastModificationDate,
        '#required' => TRUE,
      );
    }
    $form['cmis_taxonomy']['actions'] = array('#type' => 'actions');
    $form['cmis_taxonomy']['actions']['#validate'] = 'cmis_taxonomy_admin_form_validate';
    $form['cmis_taxonomy']['actions']['submit'] = array(
      '#type' => 'submit',
      '#id' => 'cmis-taxonomy-submit',
      '#value' => t('Save CMIS Taxonomy Sync'),
      '#submit' => array('cmis_taxonomy_admin_form_submit'),
    );
    if (!empty($row->cmistxid) && cmis_taxonomy_access($user)) {
      $form['cmis_taxonomy']['actions']['delete'] = array(
        '#type' => 'submit',
        '#id' => 'cmis-taxonomy-delete',
        '#value' => t('Delete CMIS Taxonomy'),
        '#submit' => array('cmis_taxonomy_admin_delete_submit'),
      );
    }
    $form['cmis_taxonomy']['actions']['cancel'] = array(
      '#type' => 'link',
      '#button_type' => 'button',
      '#title' => t('Cancel'),
      '#href' => '/admin/cmis/cmis_taxonomy',
    );
  }
  return $form;
}
/**
 * Form validation
 */
function cmis_taxonomy_admin_form_validate($form, &$form_state){
  if (empty($form_state['values']['cmistxid'])){
    form_set_error('cmis_taxonomy', 'Error Please check CMIS Taxonomy ID');
  }
}

/**
 * Form submission
 */
function cmis_taxonomy_admin_form_submit(&$form, &$form_state){
  $num_updated = db_update('cmis_taxonomy')
    ->fields(array(
      'vid' => trim($form_state['values']['vid']),
      'tid' => trim($form_state['values']['tid']),
      'cmis_name' => trim($form_state['values']['cmis_name']),
      'cmis_drupal_name' => trim($form_state['values']['cmis_drupal_name']),
      'cmis_repositoryId' => trim($form_state['values']['cmis_repositoryId']),
      'cmis_objectId' => trim($form_state['values']['cmis_objectId']),
      'cmis_tagorcat' => trim($form_state['values']['cmis_tagorcat']),
      'cmis_lastModifiedBy' => trim($form_state['values']['cmis_lastModifiedBy']),
      'cmis_lastModificationDate' => trim($form_state['values']['cmis_lastModificationDate']),
      'timestamp' => REQUEST_TIME
    ))
    ->condition('cmistxid', intval($form_state['values']['cmistxid']), '=')
    ->execute();
  drupal_set_message('Taxonomy Term '.$form_state['values']['cmis_name'].' has been updated');
  drupal_goto('/admin/cmis/cmis_taxonomy');
}

function cmis_taxonomy_admin_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $form_state['redirect'] = array('admin/cmis/cmis_taxonomy/taxonomy/' . $form_state['values']['cmistxid'] . '/delete', array('query' => $destination));
}

function cmis_taxonomy_admin_delete_confirm($form, &$form_state) {
  $query = db_select('cmis_taxonomy', 't');
  $query->condition('t.cmistxid', arg(4), '=')
    ->fields('t', array('cmistxid','vid','tid','cmis_name','cmis_drupal_name','cmis_repositoryId','cmis_objectId','cmis_tagorcat','cmis_lastModifiedBy','cmis_lastModificationDate','timestamp'));
  $results = $query->execute();
  if(!empty($results)) {
    $form['cmis_taxonomy'] = array();
    foreach ($results as $row) {
      $form['cmis_taxonomy']['cmistxid'] = array(
        '#type' => 'hidden',
        '#value' => $row->cmistxid,
        '#no_cache' => TRUE,
      );
    }
  }
  return confirm_form(
    $form,
    t('Are you sure you want to delete %title?', array('%title' => $row->cmis_drupal_name)),
    '/admin/cmis/cmis_taxonomy/taxonomy',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

function cmis_taxonomy_admin_delete_confirm_submit(&$form, &$form_state) {
  if ($form_state ['values']['confirm']) {
    $num_deleted = db_delete('cmis_taxonomy')
      ->condition('cmistxid', $form_state['values']['cmistxid'])
      ->execute();
    cache_clear_all();
    watchdog('cmis_taxonomy', '@id: deleted %title.', array('@id' => $form_state['values']['cmistxid'], '%title' => $form_state['values']['cmis_drupal_name']));
    drupal_set_message(t('@id: %title has been deleted.', array('@id' => $form_state['values']['cmistxid'], '%title' => $form_state['values']['cmis_drupal_name'])));
  }
  $form_state ['redirect'] = '/admin/cmis/cmis_taxonomy';
}