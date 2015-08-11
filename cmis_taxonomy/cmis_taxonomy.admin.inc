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
    array('data' => 'Excluded', 'field' => 'cmis_exclude', 'sort' => 'ASC'),
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
    foreach ($results as $row) {
      $rows[] = array(
        "<a href=\"/admin/cmis/cmis_taxonomy/vocabulary/edit/".$row->cmis_vid."\">".$row->cmis_vname."</a>",
        $row->cmis_repositoryId,
        $row->cmis_objectId,
        $row->cmis_exclude,
        date("j F Y", $row->timestamp)
      );
    }
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows ));
  $output .= theme('pager');

  return $output;
}



function cmis_taxonomy_vocabulary_admin_form($form, &$form_state) {
  $form = array();
  return $form;
}
/**
 * Form validation
 */
function cmis_taxonomy_vocabulary_admin_form_validate($form, &$form_state){
  if ($form_state['values']['vocabulary'] == 'no')
    form_set_error('vocabulary', '"<b>no</b>" is not a valid answer, try again.');
}

/**
 * Form submission
 */
function cmis_taxonomy_vocabulary_admin_form_submit(&$form, &$form_state){
    //
    // Here you can perform whatever action that form is made for.
    //
    drupal_set_message( 'The form has been sent. "vocabulary" has the following value: '.$form_state['values']['vocabulary'] );
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

?>