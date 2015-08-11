<?php
/**
 * Get CMIS obj with CMIS content based on CMIS SQL Query.
 *
 * @param $repository
 * @param $taxonomy
 * @param $cmis_obj
 * @param $context
 */
function cmis_taxonomy_obj($repository, $taxonomy, $cmis_obj, &$context) {
  $cmis_query = 'SELECT ';
  $tablestar = ' * ';
  $from = ' FROM ';
  $table = sprintf(' %s ', $taxonomy['cmis_type']);
  $as = ' AS c ';
  $join = ' ';
  $d = ' ' ;
  $where = ' WHERE ';
  if ($taxonomy['cmis_tagorcat'] == 'category') {
    $category = TRUE;
    $tagorcat_rule = ' IN_FOLDER ';
  }
  if ($taxonomy['cmis_tagorcat'] == 'tags') {
    $tags = TRUE;
    $tagorcat_rule = ' IN_TREE ';
  }
  $cmis_taxonomy_cron = $taxonomy['cmis_taxonomy_cron'] ? sprintf(' AND c.cmis:lastModificationDate >= \'%s\'', date_create('12 hour ago')->format('Y-m-d\TH:i:s.000-00:00')) : ' ' ;

  $cmis_query = $cmis_query . $tablestar . $from . $table . $as . $join . $where . $tagorcat_rule . sprintf(' (\'%s\') ', $cmis_obj->id) .$cmis_taxonomy_cron;

  $cmis_taxonomy = cmisapi_query($repository->repositoryId, $cmis_query, $options = array('maxItems' => $taxonomy['maxItems']));

  if(is_object($cmis_taxonomy) && ($cmis_taxonomy->numItems > 0))
    return $cmis_taxonomy;
  else {
    watchdog('cmis_taxonomy', 'No CMIS Tag/Category data exists for @cmis_obj and returned Error: @cmis_taxonomy', array('@cmis_obj' => print_r($cmis_obj,1), '@cmis_taxonomy' => print_r($cmis_taxonomy,1)), WATCHDOG_ERROR, NULL);
    return FALSE;
  }
}

/**
 * Get CMIS Category TREE obj content based on CMIS SQL Query.
 *
 * @param $repository
 * @param $taxonomy
 * @param $cmis_objectId
 * @param $context
 */
function cmis_taxonomy_terms_obj($repository, $taxonomy, $cmis_objectId, &$context) {
  $cmis_query = 'SELECT ';
  $tablestar = ' * ';
  $from = ' FROM ';
  $table = sprintf(' %s ', $taxonomy['cmis_type']);
  $as = ' AS c ';
  $join = ' ';
  $d = ' ' ;
  $where = ' WHERE ';
  $tree = ' IN_TREE ';
  $cmis_taxonomy_cron = $taxonomy['cmis_taxonomy_cron'] ? sprintf(' AND c.cmis:lastModificationDate >= \'%s\'', date_create('12 hour ago')->format('Y-m-d\TH:i:s.000-00:00')) : ' ' ;

  $cmis_query = $cmis_query . $tablestar . $from . $table . $as . $join . $where . $tree . sprintf(' (\'%s\') ', $cmis_objectId);

  $cmis_terms = cmisapi_query($repository, $cmis_query, $options = array('maxItems' => $taxonomy['maxItems']));

  if(is_object($cmis_terms) && ($cmis_terms->numItems > 0))
    return $cmis_terms;
  else {
    watchdog('cmis_taxonomy', 'No CMIS Tag/Category data exists for @cmis_obj and returned Error: @cmis_taxonomy', array('@cmis_obj' => print_r($cmis_obj,1), '@cmis_taxonomy' => print_r($cmis_terms,1)), WATCHDOG_ERROR, NULL);
    return FALSE;
  }
}

/**
 * Deletes Drupal Taxonomy referencing to CMIS deleted objects.
 *
 * @param $repository
 * @param $taxonomy
 */
function cmis_taxonomy_drupal_delete($repository, $taxonomy, $taxonomy) {
  global $conf;
  // get taxonomy list
  $cmis_taxonomy = db_query('SELECT * FROM {cmis_taxonomy} WHERE cmis_repositoryId = :repo', array(':repo' => $repository->repositoryId));

  if ($cmis_taxonomy->rowCount()) {
    $cmis_taxonomies = array();
    foreach ($cmis_taxonomy as $cmis_tax) {
      if (taxonomy_load($cmis_tax->tid))
        $cmis_taxonomies[$cmis_tax->cmis_objectId] = $cmis_tax->tid;
    }
    $cmis_tax_objectIds = array_keys($cmis_taxonomies);
    // Identify Existing CMIS Category objects in the Repo before Drupal Local DELETE
    if (!empty($cmis_tax_objectIds)) {
      $cmis_objects = cmisapi_query($repository->repositoryId, sprintf('SELECT cmis:objectId FROM %s WHERE cmis:objectId IN (\'%s\')', $taxonomy['cmis_type'], join('\',\'', $cmis_tax_objectIds)));
      foreach ($cmis_objects->objectList as $cmis_object) {
        if (array_key_exists($cmis_object->id, $cmis_taxonomies))
          unset($cmis_taxonomies[$cmis_object->id]);
      }
    }
    // delete all CMIS Object -> Drupal Category/Term References
    if (count($cmis_taxonomies) && ('deletes' === TRUE)) {
      db_delete('cmis_taxonomy')
        ->condition('tid', array_values($cmis_taxonomies), 'IN')
        ->execute();
      foreach ($cmis_taxonomies as $cmis_objectId => $drupal_tid)
        taxonomy_delete($drupal_tid);
    }
  }
}

?>