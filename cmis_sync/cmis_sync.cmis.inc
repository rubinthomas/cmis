<?php
/**
 * Handles CMIS to Drupal updates.
 *
 */
function _cmis_sync_cmis_drupal_update($context = array(), $user) {
  module_load_include('api.inc', 'cmis');
  module_load_include('inc', 'node', 'node.pages');
  global $conf;
  $sync_map = $conf['cmis_sync_map'];

  foreach ($sync_map as $node_type => $sync_map_type) {
    // check if sync is enabled for this Drupal content type
    if (!array_key_exists('enabled',  $sync_map_type) || !$sync_map_type['enabled']) {
      continue;
    }
    // merge in defaults
    $sync_map_type += array(
      'cmis_sync_type'  => $node_type,
      'fields' => array(),
      'content_field' => 'body',
      'maxItems' => 100000,
      'renditionFilter' => 'cmis:thumbnail',
      'content_format' => filter_default_format(),
      'cmis_type' => 'cmis:document',
      'cmis_versionLabel' => 'cmis:versionLabel',
      'cmis_repositoryId' => 'default',
      'subfolders' => FALSE,
      'deletes' => FALSE,
      'cmis_sync_cron' => FALSE,
    );

    try {
      // lookup CMIS repository
      $repository = cmis_get_repository($sync_map_type['cmis_repositoryId']);
      // handle CMIS updates
      _cmis_sync_cmis_drupal_handle_updates($repository, $sync_map_type, $node_type, $user);
      // handle CMIS deletes
      if ($sync_map_type['deletes']) {
        _cmis_sync_cmis_drupal_handle_deletes($repository, $sync_map_type, $node_type);
      }
    }
    catch (CMISException $e) {
      cmis_error_handler('cmis_sync', $e);
    }
  }
}

/**
 * Creates/updates Drupal nodes with CMIS content.
 *
 * @param $repository
 * @param $sync_map_type
 * @param $node_type
 */
function _cmis_sync_cmis_drupal_handle_updates($repository, $sync_map_type, $node_type, $user) {
  global $base_url;
  // get CMIS object properties
  if (isset($sync_map_type['cmis_folderId'])) {
    $cmis_folder = cmisapi_getProperties($repository->repositoryId, $sync_map_type['cmis_folderId']);
  }
  elseif (isset($sync_map_type['cmis_folderPath'])) {
    $cmis_folder = cmisapi_getObjectByPath($repository->repositoryId, $sync_map_type['cmis_folderPath']);
  }
  else {
    throw new CMISException(t("Please set `cmis_folderPath` or `cmis_folderId` properties for [@type] Drupal type.", array('@type' => print_r($node_type,1))));
  }

  // select updated objects
  $need_cm_titled = FALSE;
  $need_cm_taggable = FALSE;
  $need_cm_generalclassifiable = FALSE;
  if(array_key_exists('fields',$sync_map_type)) {
    foreach ($sync_map_type['fields'] as $field) {
      if (isset($field['cmis']) && ($field['cmis'] == "cm:title" || $field['cmis'] == "cm:description"))
        $need_cm_titled = TRUE;
      if (isset($field['cmis']) && ($field['cmis'] == "cm:category" || $field['cmis'] == "cm:categories"))
        $need_cm_generalclassifiable = TRUE;
      if (isset($field['cmis']) && ($field['cmis'] == "cm:taggable"))
        $need_cm_taggable = TRUE;
    }
  }

  //Construct CMIS SQL Query
  $cmis_query = 'SELECT ';
  $from = ' FROM ';
  $as = ' AS d ';
  $join = ' ';
  $d = ' ' ;
  $tablestar = ' * ';
  $table = sprintf(' %s ', $sync_map_type['cmis_type']);
  $where = ' WHERE ';
  $sync_subfolders_rule = $sync_map_type['subfolders'] ? ' IN_TREE ' : ' IN_FOLDER ';
  $sync_full_rule = (($sync_map_type['cmis_sync_cron'] === TRUE) ? ' ' : sprintf(' AND d.cmis:lastModificationDate >= \'%s\'', date_create('12 hour ago')->format('Y-m-d\TH:i:s.000-00:00')));

  if($need_cm_generalclassifiable) {
    $d = ' d, ';
    $join .= ' JOIN cm:generalclassifiable AS c ON d.cmis:objectId = c.cmis:objectId ';
  }

  if($need_cm_taggable) {
    $d = ' d, ';
    $join .= ' JOIN cm:taggable as tg ON d.cmis:objectId = tg.cmis:objectId ';
  }

  if($need_cm_titled) {
    $d = ' d, ';
    $join .= ' JOIN cm:titled AS t ON d.cmis:objectId = t.cmis:objectId ';
  }

  $orderby = ' ORDER BY cmis:name ASC ';

  $cmis_query = $cmis_query . $tablestar . $from . $table . $as . $join . $where . $sync_subfolders_rule . sprintf(' (%s \'%s\') ', $d ,$cmis_folder->id) .$sync_full_rule.$orderby;
  #print "<pre>cmis_query : ".$cmis_query."</pre>";

  $cmis_updates = cmisapi_query($repository->repositoryId, $cmis_query, $options = array('renditionFilter' => $sync_map_type['renditionFilter'], 'maxItems' => $sync_map_type['maxItems']));
  #print "<pre>cmis_updates : ".print_r($cmis_updates,1)."</pre>";
  #exit;

  if(is_object($cmis_updates) && ($cmis_updates->numItems > 0)) {
    foreach ($cmis_updates->objectList as $cmis_update) {
      #$renditions = cmisapi_getRenditions($repository->repositoryId, $cmis_update->properties['cmis:objectId'], $options = array('renditionFilter' => "cmis:thumbnail,alf:webpreview"));
      #$cmis_update->links = $renditions->links;
      if (is_array($cmis_update->renditions) && !empty($cmis_update->renditions)) {
        $thumb_binary = NULL;
        foreach($cmis_update->renditions as $k => $v) {
          $thumb_binary = cmisapi_getContentStream($repository->repositoryId, $cmis_update->renditions[$k]['streamId']);
          $cmis_update->properties["cmis:linkimage"] = l(
            '<img width="'.$cmis_update->renditions[$k]['width'].'" alt="'.$cmis_update->properties['cmis:name'].'" src="data:'.$cmis_update->renditions[$k]['mimetype'].';base64,'.base64_encode($thumb_binary).'" class="renditionthumbnail '.$cmis_update->renditions[$k]['kind'].'" id="'.$cmis_update->renditions[$k]['renditionDocumentId'].'" height="'.$cmis_update->renditions[$k]['height'].'">',
                  $base_url . '/cmis/browser',
            array(
                'attributes' => array(
                'id' => 'cmislink_'.$cmis_update->renditions[$k]['renditionDocumentId'],
                'class' => 'cmislink'
                ),
                'query' => array(
                'id' => $cmis_update->properties['cmis:versionSeriesId']
                ),
                'fragment' => $_SERVER['REQUEST_TIME'],
                'html' => TRUE
              )
            );

          $thumb_binary = NULL;
        }
      }
      #print "<pre>cmis_categories : ".print_r($cmis_update->properties['cm:categories'],1)."</pre>";
      #print "<pre>cmis_taggable : ".print_r($cmis_update->properties['cm:taggable'],1)."</pre>";

      // build/lookup Drupal node
      $drupal_node = _cmis_sync_cmis_drupal_prepare($repository, $sync_map_type, $node_type, $cmis_update,$user);

      // unable to map current CMIS object to any Drupal content type
      if (FALSE === $drupal_node) {
        watchdog('cmis_sync', 'Unable to map current CMIS object to any Drupal content type @cmis - @code - @message', array('@cmis'=>print_r($cmis_update,1), '@code'=>$e->getCode(), '@message'=>$e->getMessage()), WATCHDOG_WARNING);
        continue;
      }
      // mark the Drupal node in order to bypass nodeapi cmis_sync hook
      $drupal_node->cmis_sync_disabled = TRUE;
      // save Drupal node
      node_object_prepare($drupal_node);
      #print "<pre>node : ".print_r($drupal_node,1)."</pre>";
      #exit;
      node_save($drupal_node);
      if(!empty($drupal_node->nid)) {
        // update/insert changed timestamp
        $result = db_query('SELECT nid FROM {cmis_sync_node} WHERE cmis_objectId = :cid', array(':cid' => $cmis_update->properties['cmis:versionSeriesId']));
        if ($result->rowCount()) {
          db_update('cmis_sync_node')
            ->fields(array(
            'changed_timestamp' => $_SERVER['REQUEST_TIME'],
            'cmis_repositoryId' => $repository->repositoryId,
            'cmis_versionLabel' => $cmis_update->properties['cmis:versionLabel'],
            'nid' => $drupal_node->nid,
            ))
            ->condition('cmis_objectId', $cmis_update->properties['cmis:versionSeriesId'])
            ->execute();
          watchdog('cmis_sync', 'Updated nid @nid', array('@nid' => $drupal_node->nid));
        } else {
          db_insert('cmis_sync_node')
            ->fields(array(
            'nid' => $drupal_node->nid,
            'cmis_repositoryId' => $repository->repositoryId,
            'cmis_objectId' => $cmis_update->properties['cmis:versionSeriesId'],
            'cmis_versionLabel' => $cmis_update->properties['cmis:versionLabel'],
            'changed_timestamp' => $_SERVER['REQUEST_TIME'],
            ))
            ->execute();
          watchdog('cmis_sync', 'Added nid @nid', array('@nid' => $drupal_node->nid));
        }
      }
      else {
        watchdog('cmis_sync', 'Node Not Created for @cid', array('@cid' => $cmis_update->properties['cmis:versionSeriesId']));
      }
    }
  }
  else
    watchdog('cmis_sync', 'No CMIS data exists', array('@cmis_updates' => $cmis_updates), WATCHDOG_WARNING, NULL);
}

/**
 * Deletes Drupal nodes referencing to CMIS deleted objects.
 *
 * @param $repository
 * @param $sync_map_type
 */
function _cmis_sync_cmis_drupal_handle_deletes($repository, $sync_map_type, $node_type) {
  // get node list
  $cmis_nodes = array();
  $sync_nodes = db_query('SELECT nid, cmis_objectId FROM {cmis_sync_node} WHERE cmis_repositoryId = :repo', array(':repo' => $repository->repositoryId));

  if ($sync_nodes->rowCount()) {
    foreach ($sync_nodes as $sync_node) {
      if (node_load($sync_node->nid)->type == $node_type) {
        $cmis_nodes[$sync_node->cmis_objectId] = $sync_node->nid;
      }
    }
    // identify existing CMIS objects
    if (array_keys($cmis_nodes)) {
      // issue #2219095: don't try to query if we don't have any cmis nodes for this node_type: prevents 'Invalid node ref - does not contain forward slash:' server error
      $cmis_objects = cmisapi_query($repository->repositoryId, sprintf('SELECT cmis:objectId FROM %s WHERE cmis:objectId IN (\'%s\')', $sync_map_type['cmis_type'], join('\',\'', array_keys($cmis_nodes))));
      if (is_object($cmis_objects)) {
        foreach ($cmis_objects->objectList as $cmis_object) {
          if (array_key_exists($cmis_object->id, $cmis_nodes)) {
            unset($cmis_nodes[$cmis_object->id]);
          }
        }
      }
      }

    if (count($cmis_nodes)) {
      // delete CMIS - Drupal reference
      db_delete('cmis_sync_node')
        ->condition('nid', array_values($cmis_nodes), 'IN')
        ->execute();
      // delete Drupal nodes
      if (is_object($cmis_nodes)) {
        foreach ($cmis_nodes as $cmis_objectId => $drupal_nid) {
          node_delete($drupal_nid);
        }
      }
    }
  }
}

/**
 * Maps a cmis_object to a drupal node.
 *
 * @param $cmis_repository
 * @param $sync_map_type Sync rules for current type
 * @param $cmis_object
 * @return $drupal_node
 *
 * @todo
 *  Add workflow properties
 */
function _cmis_sync_cmis_drupal_prepare($repository, $sync_map_type, $node_type, $cmis_object,$user) {
  module_load_include('api.inc', 'cmis');
  $drupal_mime = array("text/plain","text/html","application/xhtml+xml","application/xml","application/xhtml");

  if ($sync_map_type['enabled']) {
    module_load_include('drupal.inc', 'cmis_sync');
    $drupal_nid = NULL;
    // Find Drupal nid in already existing 'cmis_sync_node'
    if (!array_key_exists('nid', $sync_map_type['fields'])) {
      $cmis_sync_nodes = db_query('SELECT nid FROM {cmis_sync_node} WHERE cmis_objectId = :obj', array(':obj' => $cmis_object->properties['cmis:versionSeriesId']));
      if ($cmis_sync_nodes->rowCount()) {
        foreach ($cmis_sync_nodes as $cmis_sync_node) {
          $drupal_nid = $cmis_sync_node->nid;
        }
      }
    }
    else {
      //Retrieve from cmis_object
      if(array_key_exists($sync_map_type['fields']['nid'],$cmis_object->properties))
        $drupal_nid = $cmis_object->properties[$sync_map_type['fields']['nid']];
      else
        $drupal_nid = NULL;
    }

    // Load Drupal node
    if (!empty($drupal_nid)) {
      $node = node_load($drupal_nid, NULL, $reset = TRUE);
      $node->is_new = FALSE;//FOR UPDATE NODE THIS HAS TO BE FALSE
      if(empty($node)) {
        watchdog('cmis_sync', 'Failed to Load Nid @nid', array('@nid' => $drupal_nid));
        throw new CMISException(t('Unsupported Drupal Content field type [@cmis_sync]. Check Settings Configuration.', array('@cmis_sync' => print_r($cmis_sync_nodes,1))));
      }
    }
    else // Create New Drupal node
      $node = _cmis_sync_cmis_drupal_get_node($node_type,$sync_map_type,$cmis_object->properties['cmis:name'],array(),$user);

    // fix node title
    if (!empty($cmis_object->properties['cmis:name'])) {
      $title = NULL;
      $title = filter_xss(utf8_encode($cmis_object->properties['cmis:name']));
      $node->title = drupal_set_title($title,PASS_THROUGH);
    }
    else {
      $title = NULL;
      $title = filter_xss(utf8_encode($cmis_object->properties['cmis:versionSeriesId']));
      $node->title = drupal_set_title($title,PASS_THROUGH);
    }
    // map cmis properties to drupal node fields
    foreach ($sync_map_type['fields'] as $node_field => $cmis_field) {
      $text_format = isset($cmis_field['text_format']) ? $cmis_field['text_format'] : filter_default_format();
      if (is_string($cmis_field)) {
        $node = _cmis_sync_drupal_node_field_set($node, $node_field, $cmis_object->properties[$cmis_field],$text_format);
      }
      else if (is_array($cmis_field)) {
        if (array_key_exists('cmis to drupal', $cmis_field) && $cmis_field['cmis to drupal'] === FALSE) {
          continue;
        }
        else if (!in_array($cmis_field['cmis'], array_keys($cmis_object->properties)) && !array_key_exists($cmis_field['cmis'], $cmis_object->properties) && !isset($cmis_object->properties[$cmis_field['cmis']])) {
          // if the given CMIS field is not available, display a warning to help debugging
          // (the weird logic is because sometimes $cmis_object->properties is an object, other times it is an array..?)
          watchdog('cmis_sync', "No such field '@field' for '@objid'. Fields available to CMIS sync: [@list]",
          array('@field' => $cmis_field['cmis'], '@objid' => $cmis_object->properties['cmis:versionSeriesId'], '@list' => implode(", ", array_keys($cmis_object->properties))),WATCHDOG_WARNING);
        }
        else
          $node = _cmis_sync_drupal_node_field_set($node,$cmis_field['drupal'],$cmis_object->properties[$cmis_field['cmis']],$text_format);
      }
      else {
        throw new CMISException(t('Unknown field map type. Expects "string" or "array". Received "@type"', array('@type' => gettype($cmis_field))));
      }
    }

    // load content field
    if (array_key_exists('content_field', $sync_map_type)) {
      $text_format = isset($sync_map_type['content_format']) ? $sync_map_type['content_format'] : 'full_html';
      #$cmis_content_context['file'] = array('mime' => $cmis_object->properties['cmis:contentStreamMimeType']);
      if(in_array($cmis_object->properties['cmis:contentStreamMimeType'], $drupal_mime ,TRUE)) {
        try {
          $content_stream = cmisapi_getContentStream($repository->repositoryId,$cmis_object->properties['cmis:versionSeriesId']);
          $node = _cmis_sync_drupal_node_field_set($node,$sync_map_type['content_field'], $content_stream, $text_format);
        }
        catch (CMISException $e) {
          if(isset($node->nid) && !empty($node->nid))
            watchdog('cmis_sync', 'Error retrieving content for node #@nid - @title - @code - @message',
            array('@nid'=>$node->nid, '@title'=>$node->title, '@code'=>$e->getCode(), '@message'=>$e->getMessage()), WATCHDOG_WARNING);
          else
            watchdog('cmis_sync', 'Error retrieving content for node #@objid - @code - @message',
            array('@objid'=>$cmis_object->properties['cmis:versionSeriesId'], '@code'=>$e->getCode(), '@message'=>$e->getMessage()), WATCHDOG_WARNING);
        }
      }
      else {
        $node = _cmis_sync_drupal_node_field_set($node,$sync_map_type['content_field'],' ',$text_format);
        watchdog('cmis_sync', 'Skipping Content present in {@content_field} for @objid - @mime',
            array('content_field' => $sync_map_type['content_field'], '@objid'=>$cmis_object->properties['cmis:versionSeriesId'], '@mime'=>$cmis_object->properties['cmis:contentStreamMimeType']), WATCHDOG_WARNING);
      }
    }
    // call hook_sync_cmis_drupal_prepare() hooks
    module_invoke_all('sync_cmis_drupal_prepare', $cmis_object, $node);
    node_object_prepare($node);
    return $node;
  }
  return FALSE;
}

/**
* Creates a new node according to the content type definition
*
* @param  $type
* @return new node
*/

function _cmis_sync_cmis_drupal_get_node($node_type, $sync_map_type, $title, $node_settings = array(),$user) {
  $node = new stdClass();
  //$node = (object) $node_settings;
  $node->type = $node_type;
  $node->title = $title;
  $node->language = LANGUAGE_NONE;
  $node->body[LANGUAGE_NONE][0]['value']   = NULL;
  $node->body[LANGUAGE_NONE][0]['summary'] = NULL;
  $node->body[LANGUAGE_NONE][0]['format']  = isset($sync_map_type['content_format']) ? $sync_map_type['content_format'] : filter_default_format();
  $node->language = LANGUAGE_NONE;
  $node->is_new = TRUE;
  $node->comment = variable_get('comment_' . $node_type);
  $node_type_default = variable_get('node_options_' . $node_type, array('status', 'promote','publish','revision','sticky','translate'));
  $node->validated = TRUE;
  $node->uid = $user->uid;
  $node->promote = in_array('promote', $node_type_default);
  $node->revision = in_array('revision', $node_type_default);
  #$node->status = in_array('publish', $node_type_default);
  $node->status = TRUE;
    $node->sticky = in_array('sticky', $node_type_default);
  $node->translate = in_array('translate', $node_type_default);
  return $node;
}
?>
