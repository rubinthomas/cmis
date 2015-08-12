<?php
/**
 * @file
 *   drush integration for cmis.
 */
/**
 * The CMIS plugin URI.
 */
define('CMISPHPLIB_DOWNLOAD_URI', 'https://github.com/rubinthomas/cmis-phplib/archive/master.zip');

/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * See `drush topic docs-commands` for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */

function cmis_drush_command() {
  $items = array();
  // the key in the $items array is the name of the command.
  $items['cmis-phplib'] = array(
    'callback' => 'drush_cmis_library',
    'description' => dt("Downloads the CMIS PHP library."),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap.
    'arguments' => array(
      'path' => dt('Optional. A path where to install the PHP CMIS Library plugin. If omitted Drush will use the default location.'),
    ),
    'aliases' => array('cmisphplib'),
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function cmis_drush_help($section) {
  switch ($section) {
    case 'drush:cmis-phplib':
      return dt("Downloads the CMIS PHP library from Apache Chemistry, default location is sites/all/libraries.");
  }
}
/**
 * Implements drush_MODULE_post_pm_enable().
 */
/**
 * Command to download the PHP CMIS Library plugin.
 */

function drush_cmis_library() {
  if (!drush_shell_exec('type unzip')) {
    return drush_set_error(dt('Missing dependency: unzip. Install it before using this command.'));
  }
  $args = func_get_args();
  if ($args[0]) {
    error_log($path);
    //check that the last part of the path has cmis-phplib if not, add it.
    $path = $args[0];
  }
  else {
    $path = 'sites/all/libraries/cmis-phplib';
  }
  // Create the path if it does not exist.
  if (!is_dir($path)) {
    drush_op('mkdir', $path);
    drush_log(dt('Directory @path was created', array('@path' => $path)), 'notice');
  }
  // Set the directory to the download location.
  $olddir = getcwd();
  chdir($path);
  $filename = basename(CMISPHPLIB_DOWNLOAD_URI);
  $dirname = basename(CMISPHPLIB_DOWNLOAD_URI, '.zip');
  // Remove any existing Superfish plugin directory
  if (is_dir($dirname)) {
    drush_log(dt('A existing CMIS PHP library was overwritten at @path', array('@path' => $path)), 'notice');
  }
  // Remove any existing PHP CMIS library zip archive
  if (is_file($filename)) {
    drush_op('unlink', $filename);
  }
  // Download the zip archive
  if (!drush_shell_exec('wget ' . CMISPHPLIB_DOWNLOAD_URI)) {
    drush_shell_exec('curl -O ' . CMISPHPLIB_DOWNLOAD_URI);
  }
  if (is_file($filename)) {
    // Decompress the zip archive
    drush_shell_exec('unzip -qq -o ' . $filename);
    // Remove the zip archive
    drush_op('unlink', $filename);
  }
  // Set working directory back to the previous working directory.
  chdir($olddir);
  if (is_dir($path . '/' . $dirname)) {
    drush_log(dt('Drush was unable to download the PHP CMIS Library to @path', array('@path' => $path)), 'error');
  }
  else {
    drush_log(dt('PHP CMIS Library plugin has been downloaded to @path', array('@path' => $path)), 'success');
  }
}

?>