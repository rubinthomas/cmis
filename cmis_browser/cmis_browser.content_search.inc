<?php

function cmis_browser_search($term, $path){
  module_load_include('api.inc', 'cmis');
  module_load_include('content_get.inc', 'cmis_browser');

  $parts = explode('/', $_GET['q']);
  $path = implode('/', array_slice($parts, 4));
  $args = func_get_args();

  try {
    $repository = cmis_get_repository();
    $folder_object = cmisapi_getObjectByPath($repository->repositoryId, '/'.$path);

    foreach (array('cmis:folder', 'cmis:document') as $cmis_base_type) {
      try {
        //All children of the current folder
        $cmis_objects = cmisapi_query(
          $repository->repositoryId,
          sprintf('SELECT * FROM %s WHERE IN_TREE(\'%s\') AND (cmis:name like \'%s\' OR CONTAINS(\'%s\'))', $cmis_base_type, $folder_object->id, '%'. $term .'%', $term)
        );
      }
      catch (CMISException $e) {
        cmis_error_handler('cmis_path_autocomplete', $e);
        continue;
      }
    }

  } catch (CMISException $e) {
    cmis_error_handler('cmis_browser_search', $e);
  }

  return theme('cmis_browser', array(
    'children' => $cmis_objects->objectList,
    'bcarray' => '',
    'type' => !empty($_GET['type']) ? check_plain($_GET['type']) : '',
  ));
}

?>