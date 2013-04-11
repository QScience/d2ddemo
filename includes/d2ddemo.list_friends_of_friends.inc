<?php

/**
 * @file
 * Functions directly related to the list friends of friends tab.
 */

/*
function d2d_form_add_friends_of_friends($form, &$form_state, $friends_of_friends, $db_instances) {
  $rows = array();
  $disabled = array();
  foreach ($friends_of_friends as $instance) {
    $link = '';
    if (array_key_exists($instance['url'], $db_instances)) {
      $id = $db_instances[$instance['url']];
      $link = l(t('edit'), 'admin/d2d/instances/' . $id . '/details');
      $disabled[] = $instance['url'];
    }
    $rows[$instance['url']] = array(check_plain($instance['url']), $instance['count'], $link);
  }
  $form['instances'] = array
  (
    '#type' => 'tableselect',
    '#header' => array(t('Recommended instance'), t('No. of common friends'), ''),
    '#options' => $rows,
    '#empty' => t('All friends of your friends are already your friends.'),
  );
  foreach ($disabled as $instance) {
    $form['instances'][$instance]['#disabled'] = TRUE;
  }
  if (sizeof($disabled) < sizeof($rows)) {
    $form['submit'] = array(
      '#type' => 'submit', 
      '#value' => 'Insert into database',
    );
  }
  return $form;
}
function d2d_form_add_friends_of_friends_submit($form, &$form_state) {
  $urls = _d2d_get_ids($form_state, 'instances', 0);
  $n_inserted = 0;
  foreach ($urls as $url) {
    if (d2d_check_url($url)) {
      try {
        $id = db_insert('d2d_instances')->fields(array(
            'url' => $url,
            'description' => 'N/A',
            'time_inserted' => d2d_get_time(),
            'public_key_id' => null,
          ))->execute();
        $n_inserted++;
        d2d_set_default_group_memberships($id);
      }
      catch (Exception $e) {
      }
    }
  }
  drupal_set_message(t('Inserted @count instance(s) into database.', array('@count' => $n_inserted)));
}
*/
// sort descending by second (numeric) entry, lexicographically in case of ties
function d2ddemo_list_friends($arguments, $rpc_info) {
  $friends = d2d_get_friends();
  $return_array = array();
  foreach ($friends as $friend) {
    $tmp_array = array(
      'd2d_id' => $friend['d2d_id'],
      'url' => $friend['url'],
      'name' => $friend['name'],
    );
    $imploded_tmp_array = d2d_implode($tmp_array);
    if ($imploded_tmp_array === FALSE) {
      continue;
    }
    $return_array[] = $imploded_tmp_array;
  }
  $imploded_return_array = d2d_implode($return_array);
  if ($imploded_return_array === FALSE) {
    return '';
  }
  return $imploded_return_array;
}
function _d2ddemo_compare_count_desc($a, $b) {
  return $b['count'] == $a['count'] ? strcmp($a['d2d_id'], $b['d2d_id']) : $b['count'] - $a['count'];
}
function d2ddemo_add_instance($ix) {
  if (!d2d_check_convert_int($ix) || !isset($_SESSION['d2ddemo_add_instance'][$ix])) {
    drupal_goto('admin/d2d/demo/friends_of_friends');
  }
  $_SESSION['d2d_add_instance'] = array('url' => $_SESSION['d2ddemo_add_instance'][$ix]['url'], 'd2d_id' => $_SESSION['d2ddemo_add_instance'][$ix]['d2d_id'], 'prefill' => TRUE);
  drupal_goto('admin/d2d');
}
function d2ddemo_list_friends_of_friends() {
  d2d_get_self_instance($my_id, $my_d2d_id, $my_url, $my_public_key);
  $db_instances = array();
  $result = db_select('d2d_instances', 'i')
    ->fields('i', array('id', 'url'))
    ->execute();
  $tmp_instances = d2d_get_instances(FALSE);
  $instances = array();
  foreach ($tmp_instances as $instance) {
    $instances[$instance['d2d_id']] = $instance;
  }
  $friends = d2d_get_friends();
  $build = array();
  $friends_of_friends = array();
  $rows_fof = array();
  $n_add_links = 0;
  $link_array = array();
  foreach ($friends as $friend) {
    $rows = array();
    $error_string = '';
    $res = d2d_call_secure_rpc($friend, 'd2ddemo_list_friends', array(), $error_string);
    $description = t('no friends');
    if ($res === FALSE) {
      $description = $error_string;
    }
    else {
      $exploded_res = d2d_explode($res);
      if ($exploded_res === FALSE) {
        $description = t('erroneous return:') . ' ' . $res;
      }
      else {
        foreach ($exploded_res as $record) {
          $instance = d2d_explode($record);
          if ($instance === FALSE) {
            continue;
          }
          if (!d2d_check_array($instance, array('d2d_id' => 'd2d_check_d2d_id', 'url' => 'd2d_check_url', 'name' => 'd2d_check_name'))) {
            continue;
          }
          $tmp_instance = array_key_exists($instance['d2d_id'], $instances) ? $instances[$instance['d2d_id']] : NULL;
          $link = '';
          if (!$tmp_instance) {
            $n_add_links++;
            $link = l(t('add to database'), 'admin/d2ddemo/' . $n_add_links . '/add_instance');
            $_SESSION['d2ddemo_add_instance'][$n_add_links]['url'] = $instance['url'];
            $_SESSION['d2ddemo_add_instance'][$n_add_links]['d2d_id'] = $instance['d2d_id'];
          }
          elseif (!$tmp_instance['is_me'] && !$tmp_instance['is_friend']) {
            $link = l(t('request friendship'), 'admin/d2d/instances/' . $tmp_instance['id'] . '/friendship');
          }
          elseif ($tmp_instance['is_me']) {
            $link = l(t('your instance'), 'admin/d2d/instances/' . $tmp_instance['id'] . '/details');
          }
          else {
            $link = l(t('common friend'), 'admin/d2d/instances/' . $tmp_instance['id'] . '/details');
          }
          $rows[] = array(check_plain($instance['name']) . ' <i>(' . $instance['d2d_id'] . ', ' . check_plain($instance['url']) . ')</i>', array('data' => $link, 'align' => 'right'));
          unset($instance['name']);
          $imploded_instance = d2d_implode($instance);
          if ($tmp_instance && ($tmp_instance['is_me'] || $tmp_instance['is_friend'])) {
            continue;
          }
          $link_array[$imploded_instance] = $link;
          if (array_key_exists($imploded_instance, $friends_of_friends)) {
            ++$friends_of_friends[$imploded_instance];
          }
          else {
            $friends_of_friends[$imploded_instance] = 1;
          }
        }
      }
    }
    $rows_fof[] = array(
      array(
        'data' => check_plain($friend['name']),
        'style' => 'font-weight: bold',
      ),
      '',
    );
    if (empty($rows)) {
      $rows_fof[] = array('&nbsp;&nbsp;&nbsp;' . check_plain($description), '');
    }
    else {
      foreach ($rows as $row) {
        $row[0] = '&nbsp;&nbsp;&nbsp;' . $row[0];
        $rows_fof[] = $row;
      }
    }
  }
  $build['friends_of_friends'] = array(
    '#title' => t('Friends of friends'),
    '#type' => 'fieldset',
  );
  $build['friends_of_friends'][] = array(
    '#theme' => 'table',
    '#header' => array(t('URL'), ''),
    '#rows' => $rows_fof,
    '#empty' => t('No friends found in the database'),
  );
  $filtered_friends_of_friends = array();
  foreach ($friends_of_friends as $key => $value) {
    $instance = d2d_explode($key);
    $filtered_friends_of_friends[$key] = array('d2d_id' => $instance['d2d_id'], 'url' => $instance['url'], 'count' => $value, 'link' => $link_array[$key]);
  }
  usort($filtered_friends_of_friends, '_d2ddemo_compare_count_desc');
  $build['recommondations'] = array(
    '#title' => t('Recommondations'),
    '#type' => 'fieldset',
  );
  $build['recommondations'][] = array(
    '#theme' => 'table',
    '#header' => array(t('Recommended instance'), t('URL'), t('No. of common friends'), t('Operation')),
    '#rows' => $filtered_friends_of_friends,
    '#empty' => t('All friends of your friends are already your friend.'),
  );
  return $build;
} 
