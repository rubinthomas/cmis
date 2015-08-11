<?php
// CMIS_SPACESTORE string is based on data from CMIS Document Object to truncate in order to get CatObjId
define('CMIS_SPACESTORE', 'workspace://SpacesStore/');
/**
 * Lookup a CMIS Object ID in Drupal Lookup Table
 *
 * @param &$node reference
 * @param $field_name
 * @param $field_value
 * @param $text_format
 * @return &$node reference
 */
function cmis_taxonomy_lookup(&$node, $field_name = 'tags', $field_value = NULL) {
  $terms = NULL;

  if(!empty($field_value)) {
    //Check Lookup Table
    if(is_array($field_value)) {
      $field_value = str_replace(CMIS_SPACESTORE,'',$field_value);
      foreach($field_value as $k => $cat_objId) {
        $cat = cmis_taxonomy_lookup_term($cat_objId);
        if(!empty($cat)) {
          $vid = array_keys($cat);
          $tid = array_values($cat);
          $terms[$vid[0]][$tid[0]] = $tid[0];
        }
        $cat = $tid = $vid = NULL;
      }
    }
    else {
      $cat_objId = str_replace(CMIS_SPACESTORE,'',$field_value);
      $terms = cmis_taxonomy_lookup_term($cat_objId);
    }

    if(is_array($terms) && !empty($terms)) {
      $vids = array_keys($terms);
      $tids = array_values($terms);

      if((strpos($field_name, 'tags')!== false) || (strpos($field_name, '1')!== false)) {
        foreach ($tids as $k => $tid) {
          if(is_array($terms[$vids[$k]]) && !empty($terms[$vids[$k]])) {
            foreach($terms[$vids[$k]] as $key => $value)
              $node->{$field_name}[LANGUAGE_NONE][$key]['tid'] = $value;
          }
          else
            $node->{$field_name}[LANGUAGE_NONE][0]['tid'] = $terms[$vids[$k]];
        }
        if(is_array($node->{$field_name}) && !empty($node->{$field_name})) {
          $unsorted = $node->{$field_name}[LANGUAGE_NONE];
          $sorted = array_unique($unsorted, SORT_REGULAR);
          $node->{$field_name}[LANGUAGE_NONE] = $sorted;
        }
      }
      else if(is_array($vids) && !empty($vids)) {
        foreach ($vids as $k => $vid) {
          $str = strval($vids[$k]);
          if (!empty($str) && strpos($field_name, $str)!== false) {
            if(is_array($terms[$str]) && !empty($terms[$str])) {
              foreach($terms[$vids[$k]] as $key => $value)
                $node->{$field_name}[LANGUAGE_NONE][$key]['tid'] = $value;
            }
            else
              $node->{$field_name}[LANGUAGE_NONE][0]['tid'] = $terms[$vids[$k]];
          }
          $str = NULL;
        }
      }
      else
        unset($node->{$field_name});
    }
    else
      unset($node->{$field_name});
  }
  return $node;
}

/**
 * Lookup a term based on CMIS Cat Object ID in Drupal Lookup Table
 *
 * @param $cat_objId
 * @return return $term or NULL
 */
function cmis_taxonomy_lookup_term($cat_objId=NULL) {
  $term = NULL;
  if(!empty($cat_objId)) {
    $cmis_taxonomy = db_query('SELECT * FROM {cmis_taxonomy} WHERE cmis_objectId = :cmis_objectId', array(':cmis_objectId' => $cat_objId));
    if ($cmis_taxonomy->rowCount()) {
      $cmis_taxonomy = $cmis_taxonomy->fetchObject();
      if($cmis_taxonomy->tid > 0) {
        $term[$cmis_taxonomy->vid] = $cmis_taxonomy->tid;
        return $term;
      }
      else
        return NULL;
    }
    else {
      watchdog('cmis_taxonomy', 'No CMIS Tag/Category data exists for @cat_objId ', array('@cat_objId' => print_r($cat_objId,1)), WATCHDOG_ERROR, NULL);
      return NULL;
    }
  }
  else
    return NULL;
}

?>