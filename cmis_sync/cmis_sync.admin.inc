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
	if(array_key_exists('cmis_sync_map',$conf) && is_array($conf['cmis_sync_map']))	{
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
	
	
	/*
	* Test Code for functions written
	*/
	return $ret."</div>\n";
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
	/*
	$form['filter']['filter_cmisid'] = array(
      '#type' => 'textfield',
      '#title' => t('CMIS ID'),
      '#size' => 5,
    );
    $form['filter']['filter_nid'] = array(
      '#type' => 'textfield',
      '#title' => t('Drupal Node ID'),
      '#size' => 5,
    );
	*/
    $form['filter']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    );
	$form['filter']['reset'] = array(
	  '#type' => 'submit',
	  '#value' => t('Reset'),
	  '#markup' => l(t('Reset'), 'admin/cmis/cmis_sync/cmis/errors'),
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
	
	/*
	SELECT * FROM cmis_sync_node WHERE nid NOT IN ( SELECT nid FROM node)
	*/
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
			$rows[] = array("<a href=\"admin/cmis/cmis_sync/cmis/errors/".$row->id."/edit\">".$row->id."</a>",
						"<a href=\"/node/".$row->nid."/edit\">".$row->nid."</a>",
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

?>