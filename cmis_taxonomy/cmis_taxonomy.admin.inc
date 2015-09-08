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



function cmis_taxonomy_vocabulary_admin_form($form, &$form_state) {
  $query = db_select('cmis_vocab', 'c');
  $query->condition('c.cmis_vid', arg(5), '=')
    ->fields('c', array('cmis_vid', 'vid', 'cmis_vname', 'cmis_repositoryId', 'cmis_objectId', 'cmis_exclude', 'timestamp'));
  $results = $query->execute();
  $form = array();
  if(!empty($results)) {
    $form['cmis_taxonomy'] = array();
    foreach ($results as $row) {
      $form['cmis_taxonomy']['cmis_vid'] = array(
        '#type' => 'hidden',
        '#value' => $row->cmis_vid,
        '#no_cache' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_vname'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS Vocabulary',
        '#value' => $row->cmis_vname,
        '#disabled' => TRUE,
        '#no_cache' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_objectId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS ObjectID',
        '#value' => $row->cmis_objectId,
        '#no_cache' => TRUE,
      );
      $form['cmis_taxonomy']['cmis_repositoryId'] = array(
        '#type' => 'textfield',
        '#title' => 'CMIS RepositoryID',
        '#value' => $row->cmis_repositoryId,
        '#no_cache' => TRUE,
      );
      $options = array(0 => 'Included', 1 => 'Excluded');
      $form['cmis_taxonomy']['cmis_exclude'] = array(
        '#type' => 'select',
        '#title' => 'CMIS Synced',
        '#options' => $options,
        '#default_value' => $row->cmis_exclude,
        '#no_cache' => TRUE,
      );
    }
    $form['cmis_taxonomy']['actions'] = array('#type' => 'actions');
    $form['cmis_taxonomy']['actions']['submit'] = array(
      '#type' => 'submit',
      '#id' => 'cmis-taxonomy-vocabulary-submit',
      '#value' => t('Save CMIS Taxonomy Vocabulary Sync'),
      '#submit' => array('cmis_taxonomy_vocabulary_admin_form_submit'),
    );
    $form['cmis_taxonomy']['actions']['cancel'] = array(
      '#type' => 'link',
      '#button_type' => 'button',
      '#title' => t('Cancel'),
      '#href' => '/admin/cmis/cmis_taxonomy/vocabulary',
    );
  }
  return $form;
}
/**
 * Form validation
 */
function cmis_taxonomy_vocabulary_admin_form_validate($form, &$form_state){
  if (empty($form_state['values']['cmis_vid'])){
    form_set_error('cmis_taxonomy', 'Error Please check Vocabulary ID');
  }
}

/**
 * Form submission
 */
function cmis_taxonomy_vocabulary_admin_form_submit(&$form, &$form_state){
  $num_updated = db_update('cmis_vocab')
    ->fields(array(
      'cmis_repositoryId' => trim($form_state['values']['cmis_repositoryId']),
      'cmis_objectId' => trim($form_state['values']['cmis_objectId']),
      'cmis_exclude' => intval($form_state['values']['cmis_exclude']),
      'timestamp' => REQUEST_TIME
    ))
    ->condition('cmis_vid', intval($form_state['values']['cmis_vid']), '=')
    ->execute();
  drupal_set_message('Vocabulary '.$form_state['values']['cmis_vname'].' has been updated');
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

