<?php

/**
 * Handles Drupal to CMIS updates.
 */
function _cmis_sync_drupal_cmis_update($node, $op) {
  global $conf;
  $cmis_node_type = FALSE;
  $cmis_node = NULL;
  if(array_key_exists('cmis_sync_map',$conf) && is_array($conf['cmis_sync_map'])) {
    if (!empty($conf['cmis_sync_map'])) {
      $sync_map = $conf['cmis_sync_map'];
      if(array_key_exists($node->type,$sync_map)) {
        $cmis_node_type = TRUE;
        if(array_key_exists('drupal_to_cmis', $sync_map[$node->type])) {
          $cmis_node[$node->type] = $sync_map[$node->type]['drupal_to_cmis'];
          /* Optional Check to be done for each field
          if(array_key_exists('fields',$sync_map[$node->type])) {
            foreach ($sync_map[$node->type]['fields'] as $drupal_field => $cmis_field) {
              if (is_array($cmis_field) && array_key_exists('drupal to cmis', $cmis_field)) {
                $cmis_node[$drupal_field] = $cmis_field['drupal to cmis'];
              }
            }
          }
          */
        }
      }
    }
  }

  if($cmis_node_type === TRUE) {
    module_load_include('api.inc', 'cmis');
    $cmis_object = _cmis_sync_drupal_cmis_prepare($node);
    if(!array_key_exists($node->type,$cmis_node) && ($cmis_node[$node->type] === FALSE)) {
      watchdog('cmis_sync', 'Node @nid based on @nodetype Cannot perform operation: {@op}
      on the CMIS Repository as {Drupal to CMIS} is set FALSE',
      array('@nid' => $node->nid, '@nodetype' => $node->nodetype, '@op' => $op), WATCHDOG_INFO, NULL);
      return;
    }
    else {
      if (is_object($cmis_object) && !isset($node->cmis_sync_disabled)) {
        switch ($op) {
          case 'insert' :
            try {
              $new_cmis_object = cmisapi_createDocument($cmis_object->repositoryId,$cmis_object->folderId,$cmis_object->name,$cmis_object->properties,$cmis_object->content,$cmis_object->contentStreamMimeType);
              // saving CMIS reference id in {cmis_sync_node} table
              if(is_object($new_cmis_object)) {
                db_insert('cmis_sync_node')->fields(array(
                  'nid' => $node->nid,
                  'cmis_repositoryId' => $cmis_object->repositoryId,
                  'cmis_objectId' => $new_cmis_object->properties['cmis:versionSeriesId'],
                  'cmis_versionLabel' => $new_cmis_object->properties['cmis:versionLabel'],
                  'changed_timestamp' => $_SERVER['REQUEST_TIME'],
                ))->execute();
                watchdog('cmis_sync', 'Synced @nid', array('@nid' => $node->nid));
              }
            }
            catch (CMISException $e) {
              cmis_error_handler('cmis_sync_nodeapi', $e);
              watchdog('cmis_sync', 'cmis_sync_nodeapi Error: @e', array('@e' => print_r($e,1)), WATCHDOG_WARNING, NULL);
              return;
            }
            break;
          case 'update' :
            // send updates only if the current node has a CMIS reference id.
            $result = db_query('SELECT cmis_objectId FROM {cmis_sync_node} WHERE nid = :nid', array(':nid' => $node->nid));
            if(!empty($result)) {
              foreach($result as $row) {
                try {
                  // updating CMIS reference object content stream
                  if (!empty($cmis_object->content))
                    cmisapi_setContentStream($cmis_object->repositoryId, $row->cmis_objectId, $cmis_object->content, $cmis_object->contentStreamMimeType);

                  // updating CMIS properties
                  cmisapi_updateProperties($cmis_object->repositoryId, $row->cmis_objectId, $cmis_object->properties);
                  // update data that may have changed
                  db_query('UPDATE {cmis_sync_node} SET changed_timestamp = :changed, cmis_versionLabel= :version, cmis_repositoryId = :repositoryId WHERE nid = :nid',
                  array(':changed' => $_SERVER['REQUEST_TIME'], ':version' => $cmis_object->properties['cmis:versionLabel'], ':repositoryId' => $cmis_object->repositoryId, ':nid' => $node->nid));
                  watchdog('cmis_sync', 'Updated @nid', array('@nid' => $node->nid));
                }
                catch (CMISException $e) {
                  watchdog('cmis_sync', 'cmis_sync_nodeapi Error: @e for Node: @nid ', array('@e' => print_r($e,1),'@nid' => $node->nid), WATCHDOG_WARNING, NULL);
                  cmis_error_handler('cmis_sync_nodeapi', $e);
                  return;
                }
              }
            }
            else {
              watchdog('cmis_sync', '@nid does not exists', array('@nid' => $node->nid), WATCHDOG_WARNING, NULL);
              return;
            }
            break;
          case 'delete' :
            $result = db_query('SELECT cmis_objectId FROM {cmis_sync_node} WHERE nid = :nid', array(':nid' => $node->nid));
            if (!empty($result)) {
              foreach ($result as $row) {
                try {
                  // delete CMIS reference object content
                  if($cmis_object->sync_deletes) {
                    $cmis_object_properties = cmisapi_getProperties($cmis_object->repositoryId, $row->cmis_objectId);
                    cmisapi_deleteObject($cmis_object->repositoryId, $cmis_object_properties->properties['cmis:versionSeriesId']);
                  }
                  else
                    watchdog('cmis_sync', 'cmis_sync_nodeapi sync_deletes Not Supported: @e', array('@e' => print_r($e,1)), WATCHDOG_WARNING, NULL);

                  // delete sync refs
                  db_delete('cmis_sync_node')
                  ->condition('nid', $node->nid)
                  ->execute();
                  watchdog('cmis_sync', 'Deleted @nid', array('@nid' => $node->nid));
                }
                catch (CMISException $e) {
                  cmis_error_handler('cmis_sync_nodeapi', $e);
                  watchdog('cmis_sync', 'cmis_sync_nodeapi Error: @e', array('@e' => print_r($e,1)), WATCHDOG_WARNING, NULL);
                  return;
                }
              }
            }
            else {
              watchdog('cmis_sync', 'Node/cmis_objectId not found: @nid', array('@nid' => $node->nid), WATCHDOG_WARNING, NULL);
              return;
            }
            break;
          default:
            watchdog('cmis_sync', 'Operation Not Permitted: @op', array('@op' => $op), WATCHDOG_WARNING, NULL);
            return;
        }
      }
      else {
        watchdog('cmis_sync', 'Node cmis_sync is disabled @nid', array('@nid' => $node->nid), WATCHDOG_WARNING, NULL);
        return;
      }
    }
  }
  else {
    watchdog('cmis_sync', 'Node Not a cmis_node_type @nid, @nodetype', array('@nid' => $node->nid, '@nodetype' => $node->nodetype), WATCHDOG_INFO, NULL);
    return;
  }
}

/**
 * Maps a drupal node to a cmis_object
 *
 * @param $node
 * @return stdClass $cmis_object wrapper
 */
function _cmis_sync_drupal_cmis_prepare($node) {
  global $conf;
  $sync_map = $conf['cmis_sync_map'];

  // is cmis sync enabled for this node type?
  if (!empty($sync_map[$node->type]) && $sync_map[$node->type]['enabled']) {
    $cmis_object = new stdClass();

    // merge in defaults
    $sync_map[$node->type] += array(
      'fields' => array(),
      'content_field' => 'body',
      'cmis_type' => 'cmis:document',
    'cmis_versionLabel' => 'cmis:versionLabel',
      'cmis_repositoryId' => 'default',
      'subfolders' => FALSE,
      'deletes' => FALSE,
      'cmis_sync_cron' => FALSE,
    );

    // CMIS destination type
    $cmis_object->type = $sync_map[$node->type]['cmis_type'];

    // CMIS destination type
    $cmis_repository = cmis_get_repository($sync_map[$node->type]['cmis_repositoryId']);
    $cmis_object->repositoryId = $cmis_repository->repositoryId;

    // CMIS destination folder id
    if (isset($sync_map[$node->type]['cmis_folderId'])) {
      $cmis_object->folderId = $sync_map[$node->type]['cmis_folderId'];
    }
    elseif (isset($sync_map[$node->type]['cmis_folderPath'])) {
      $folder = cmisapi_getObjectByPath($cmis_repository->repositoryId, $sync_map[$node->type]['cmis_folderPath']);
      $cmis_object->folderId = $folder->id;
    }
    else {
      throw new CMISException(t("Please set `cmis_folderPath` or `cmis_folderId` properties for [@type] Drupal type.", array(
        '@type' => $node->type
      )));
    }

    // map Drupal node fields to cmis object properties
    $cmis_object->properties = array();
    foreach ($sync_map[$node->type]['fields'] as $drupal_field => $cmis_field) {
      if (is_string($cmis_field)) {
        $cmis_object->properties[$cmis_field] = _cmis_sync_drupal_node_field_value($node, $drupal_field);
      }
      elseif (is_array($cmis_field)) {
        if (array_key_exists('drupal to cmis', $cmis_field) && $cmis_field['drupal to cmis'] === FALSE) {
          continue;
        }
        $cmis_object->properties[$cmis_field['cmis']] = _cmis_sync_drupal_node_field_value($node, $cmis_field['drupal']);
      }
      else {
        throw new CMISException(t('Unknown field map type. Expects "string" or "array". Received @type', array('@type' => gettype($cmis_field))));
      }
    }

    // map Drupal node field as object's content
    if (array_key_exists('content_field', $sync_map[$node->type])) {
      $cmis_object->content = _cmis_sync_drupal_node_field_value($node, $sync_map[$node->type]['content_field']);

      // setting content's content-type
      $cmis_object->properties['content-type'] = 'text/html';
      $fields = field_info_instances('node', $node->type);
      $content_field_name = $sync_map[$node->type]['content_field'];
      if (array_key_exists($content_field_name, $fields)) {
        if (in_array($fields[$content_field_name]['widget']['type'], array('image_image', 'file_generic'))) {
          $items = field_get_items('node', $node, $content_field_name);
          $file = file_load($items[0]['fid']);
          $cmis_object->properties['content-type'] = $file->filemime;
        }
      }
    }

    // clean up properties array
    $cmis_object->contentStreamMimeType = $cmis_object->properties['content-type'];
    unset($cmis_object->properties['content-type']);

    // make sure that cmis:name property is filled.
    if (!array_key_exists('cmis:name', $cmis_object->properties)) {
      $cmis_object->properties['cmis:name'] = $node->title;
    }

    $cmis_object->name = $cmis_object->properties['cmis:name'];

    // sync deletes flag
    $cmis_object->sync_deletes = $sync_map[$node->type]['deletes'];

    // call hook_sync_drupal_cmis_prepare() hooks
    module_invoke_all('sync_drupal_cmis_prepare', $node, $cmis_object);

    return $cmis_object;
  }

  return FALSE;
}

/**
 * Utility that Gets node field value.
 * Supports regular, text and filefield fields types.
 */
function _cmis_sync_drupal_node_field_value(&$node, $field_name, $text_format = 'filtered_html') {
  $field_value = NULL;
  if (is_object($node) && property_exists ( $node , 'type' )) {
    $fields = field_info_instances('node', $node->type);
    if (array_key_exists($field_name, $fields)) {
      $content_field = $field_name;
      switch($fields[$field_name]['widget']['type']) {
        case 'image_image':
        case 'file_generic':
          $items = field_get_items('node', $node, $field_name, LANGUAGE_NONE);
          if (is_array($items)) {
              $items = reset($items);
            $file = file_load($items[0]['fid']);
            if (!empty($file->uri))
              file_put_contents($file->uri, $field_value);
            else
              watchdog('cmis_sync', 'No File exists @file', array('@file' => $file), WATCHDOG_WARNING, NULL);
          }
          else
            watchdog('cmis_sync', 'No File exists in @nid for [@field_name]', array('@nid' => $node->nid, '@field_name' => $field_name), WATCHDOG_WARNING, NULL);
          break;
        case 'text':
        case 'text_textfield':
        case 'text_textarea':
        case 'text_textarea_with_summary':
        case 'date':
        case 'datestamp':
        case 'datetime':
        case 'number_decimal':
        case 'number_float':
        case 'number_integer':
          if (is_array($content_field) && array_key_exists('value', $content_field))
            $field_value = utf8_encode($content_field[LANGUAGE_NONE][0]['value']);
          break;
        case 'options_buttons':
        case 'options_select':
        case 'taxonomy_autocomplete':#need it for taxomony or defined options selected list
            if(module_exists('cmis_taxonomy')) {
              module_load_include('drupal.inc', 'cmis_taxonomy');
              $node = cmis_taxonomy_lookup($node, $field_name, $field_value);
            }
            else {
              watchdog('cmis_sync', 'Taxomony or Options selected list is Unsupported in CMIS sync Drupal field type [@field_type] for field [@field_name]', array('@field_name' => $field_name, '@field_type' => $fields[$field_name]['widget']['type']), WATCHDOG_EMERGENCY, NULL);
            }
            break;
        case 'link_field':
          if(module_exists('link')) {
            $node->$field_name = array();
            if(is_array($field_value) && !empty($field_value)) {
              foreach($field_value as $k => $v) {
                $node->{$field_name}[LANGUAGE_NONE][$k]['url'] = 'http://www.trunet.local/cmis/browser?id='.utf8_encode(trim($v));
                $node->{$field_name}[LANGUAGE_NONE][$k]['title'] = NULL;
                $node->{$field_name}[LANGUAGE_NONE][$k]['attributes'] = array();
              }
            }
            else {
              $node->{$field_name}[LANGUAGE_NONE][0]['url'] = 'http://www.trunet.local/cmis/browser?id='.utf8_encode(trim($field_value));
              $node->{$field_name}[LANGUAGE_NONE][0]['title'] = NULL;
              $node->{$field_name}[LANGUAGE_NONE][0]['attributes'] = array();
            }
          }
          else {
            watchdog('cmis_sync', 'Link is Unsupported in CMIS sync Drupal field type [@field_type] for field [@field_name]', array('@field_name' => $field_name, '@field_type' => $fields[$field_name]['widget']['type']), WATCHDOG_EMERGENCY, NULL);
          }
          break;
        default:
          watchdog('cmis_sync', 'Unsupported sync field type [@field_type] for field [@field_name]', array(
            '@field_name' => $field_name,
            '@field_type' => $fields[$field_name]['widget']['type']
          ), WATCHDOG_ALERT, NULL);
          break;
      }
      return $field_value;
    }
    else {
      if (array_key_exists($field_name, $node))
        return utf8_encode($node->{$field_name});
      return $field_value;
    }
  }
  else
    return $field_value;
}

/**
 * Utility that Sets node field value.
 * Supports regular, text and filefield fields types.
 */
function _cmis_sync_drupal_node_field_set(&$node, $field_name, $field_value = NULL, $text_format = 'full_html') {
  if (isset($node) && is_object($node) && property_exists ( $node , 'type' )) {
    $fields = field_info_instances('node', $node->type);

    if (array_key_exists($field_name, $fields)) {
      switch($fields[$field_name]['widget']['type']) {
        case 'image_image':
        case 'file_generic':
          if ($field_value != NULL) {
            $field_info = field_info_field($field_name);
            $instance_info = field_info_instance('node', $field_name, $node->type);
            $path = $field_info['settings']['uri_scheme'] . '://';
            if (strlen($instance_info['settings']['file_directory']) > 0) {
              $path .= $instance_info['settings']['file_directory'] . '/';
              if (!is_dir($path)) {
                drupal_mkdir($path, NULL, TRUE);
              }
            }
            // add an image
            $filename = $path . uniqid('cmis_sync_');
            $file = file_save_data($field_value, $filename);
            if ($file) {
              $file->display = 1;
              $node->{$field_name}[LANGUAGE_NONE][0] = (array)$file;
            }
          }
          break;
        case 'text':
        case 'text_textfield':
        case 'text_textarea':
        case 'text_textarea_with_summary':
          $node->$field_name = array();
          if(is_array($field_value) && !empty($field_value)) {
            foreach($field_value as $k => $v) {
              $node->{$field_name}[LANGUAGE_NONE][$k]['value'] = utf8_encode(trim($v));
              $node->{$field_name}[LANGUAGE_NONE][$k]['summary'] = NULL;
              $node->{$field_name}[LANGUAGE_NONE][$k]['format'] = $text_format;
            }
          }
          else {
            $node->{$field_name}[LANGUAGE_NONE][0]['value'] = utf8_encode(trim($field_value));
            $node->{$field_name}[LANGUAGE_NONE][0]['summary'] = NULL;
            $node->{$field_name}[LANGUAGE_NONE][0]['format'] = $text_format;
          }
          break;
        case 'date':
        case 'datestamp':
        case 'datetime':
        case 'number_decimal':
        case 'number_float':
        case 'number_integer':
          $node->$field_name = array();
          if(is_array($field_value) && !empty($field_value)) {
            foreach($field_value as $k => $v) {
              $node->{$field_name}[LANGUAGE_NONE][$k]['value'] = utf8_encode(trim($v));
              #$node->{$field_name}[LANGUAGE_NONE][$k]['summary'] = NULL;
              $node->{$field_name}[LANGUAGE_NONE][$k]['format'] = $text_format;
            }
          }
          else {
            $node->{$field_name}[LANGUAGE_NONE][0]['value'] = utf8_encode(trim($field_value));
            #$node->{$field_name}[LANGUAGE_NONE][0]['summary'] = NULL;
            $node->{$field_name}[LANGUAGE_NONE][0]['format'] = $text_format;
          }
          break;
        case 'options_buttons':
        case 'options_select':
        case 'taxonomy_autocomplete':#need it for taxomony or defined options selected list
          if(module_exists('cmis_taxonomy')) {
            module_load_include('drupal.inc', 'cmis_taxonomy');
            $node = cmis_taxonomy_lookup($node, $field_name, $field_value);
          }
          else {
            watchdog('cmis_sync', 'Taxomony or Options selected list is Unsupported in CMIS sync Drupal field type [@field_type] for field [@field_name]', array('@field_name' => $field_name, '@field_type' => $fields[$field_name]['widget']['type']), WATCHDOG_EMERGENCY, NULL);
          }
          break;
        case 'link_field':
          if(module_exists('link')) {
            $node->$field_name = array();
            if(is_array($field_value) && !empty($field_value)) {
              foreach($field_value as $k => $v) {
                $node->{$field_name}[LANGUAGE_NONE][$k]['url'] = 'http://www.trunet.local/cmis/browser?id='.utf8_encode(trim($v));
                $node->{$field_name}[LANGUAGE_NONE][$k]['title'] = NULL;
                $node->{$field_name}[LANGUAGE_NONE][$k]['attributes'] = array();
              }
            }
            else {
              $node->{$field_name}[LANGUAGE_NONE][0]['url'] = 'http://www.trunet.local/cmis/browser?id='.utf8_encode(trim($field_value));
              $node->{$field_name}[LANGUAGE_NONE][0]['title'] = NULL;
              $node->{$field_name}[LANGUAGE_NONE][0]['attributes'] = array();
            }
          }
          else {
            watchdog('cmis_sync', 'Link is Unsupported in CMIS sync Drupal field type [@field_type] for field [@field_name]', array('@field_name' => $field_name, '@field_type' => $fields[$field_name]['widget']['type']), WATCHDOG_EMERGENCY, NULL);
          }
          break;
        default:
          watchdog('cmis_sync', 'Unsupported CMIS Sync for Drupal content field type [@field_type] for field [@field_name]', array('@field_name' => $field_name,  '@field_type' => $fields[$field_name]['widget']['type']), WATCHDOG_EMERGENCY, NULL);
          break;
      }
      return $node;
    }
    else {
      if(is_array($field_value) && !empty($field_value)) {
        foreach($field_value as $k => $v) {
          if($field_name == 'title')
            $node->title = utf8_encode(trim($v));
          else
            $node->{$field_name}[$k] = utf8_encode(trim($v));
        }
      }
      else if($field_name == 'title' && !empty($field_value))
        $node->title = utf8_encode(trim($field_value));
      else
        $node->{$field_name} = utf8_encode(trim($field_value));

      watchdog('cmis_sync', 'Drupal field [@field_name] not present in Node Field Entity', array('@field_name' => $field_name), WATCHDOG_INFO, NULL);
      return $node;
    }
  }
  else {
    watchdog('cmis_sync', 'Node configuration error: @node', array('@node' => print_r($node,1)), WATCHDOG_CRITICAL, NULL);
    return NULL;
  }
}

?>
