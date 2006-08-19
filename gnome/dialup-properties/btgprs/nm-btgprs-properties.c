/* nm-btgprs-properties.c : GNOME UI dialogs for configuring btgprs connections
 *
 * Copyright (C) 2005-2006 Tim Niemueller <tim@niemueller.de>
 * Based on work by David Zeuthen, <davidz@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>
#include <string.h>
#include <stdlib.h>
#include <glade/glade.h>

#define NM_DIALUP_API_SUBJECT_TO_CHANGE

#include "nm-dialup-ui-interface.h"

typedef struct _NetworkManagerDialupUIImpl NetworkManagerDialupUIImpl;

struct _NetworkManagerDialupUIImpl {
  NetworkManagerDialupUI parent;

  NetworkManagerDialupUIDialogValidityCallback callback;
  gpointer callback_user_data;

  gchar    *last_fc_dir;

  GladeXML *xml;

  GtkWidget *widget;

  GtkEntry       *w_connection_name;
  GtkEntry       *w_device;
  GtkEntry       *w_channel;
  GtkEntry       *w_apn;
  GtkComboBox    *w_baudrate;
  GtkComboBox    *w_flowcontrol;
  GtkExpander    *w_comp_info_expander;
  GtkCheckButton *w_use_gprs_header_comp;
  GtkCheckButton *w_use_gprs_data_comp;
  GtkButton      *w_import_button;
};


static const char *
baudrate_itos(const gint baudrate_index)
{
  switch ( baudrate_index ) {
  case 0:
    return "460800";
  case 1:
    return "230400";
  case 2:
    return "115200";
  case 3:
    return "57600";
  case 4:
    return "38400";
  case 5:
    return "19200";
  case 6:
    return "9600";
  case 7:
    return "4800";
  case 8:
    return "2400";
  case 9:
    return "1200";
  case 10:
    return "300";
  default:
    // seems to be a popular default
    return "57600";
  }
}


static gint
baudrate_stoi(const gchar *baudrate_string)
{
  gint baudrate_index = -1;

  if ( strcmp (baudrate_string, "460800") == 0 ) {
    baudrate_index = 0;
  } else if ( strcmp (baudrate_string, "230400") == 0 ) {
    baudrate_index = 1;
  } else if ( strcmp (baudrate_string, "115200") == 0 ) {
	baudrate_index = 2;
  } else if ( strcmp (baudrate_string, "57600") == 0 ) {
    baudrate_index = 3;
  } else if ( strcmp (baudrate_string, "38400") == 0 ) {
    baudrate_index = 4;
  } else if ( strcmp (baudrate_string, "19200") == 0 ) {
    baudrate_index = 5;
  } else if ( strcmp (baudrate_string, "9600") == 0 ) {
    baudrate_index = 6;
  } else if ( strcmp (baudrate_string, "4800") == 0 ) {
    baudrate_index = 7;
  } else if ( strcmp (baudrate_string, "2400") == 0 ) {
    baudrate_index = 8;
  } else if ( strcmp (baudrate_string, "1200") == 0 ) {
    baudrate_index = 9;
  } else if ( strcmp (baudrate_string, "300") == 0 ) {
    baudrate_index = 10;
  }

  return baudrate_index;
}


static const gchar *
flowcontrol_itos(const gint flowcontrol_index)
{
  switch (flowcontrol_index) {
  case 1:
    return "xonxoff";
  case 2:
    return "crtscts";
  default:
    return "none";
  }
}


static gint
flowcontrol_stoi(const gchar *flowcontrol_string)
{
  gint flowcontrol_index = 0;

  if ( strcmp (flowcontrol_string, "xonxoff") == 0 ) {
    flowcontrol_index = 1;
  } else if ( strcmp (flowcontrol_string, "crtscts") == 0 ) {
    flowcontrol_index = 2;
  }

  return flowcontrol_index;
}


static void
btgprs_clear_widget (NetworkManagerDialupUIImpl *impl)
{
  gtk_entry_set_text (impl->w_connection_name, "");
  gtk_entry_set_text (impl->w_channel, "");
  gtk_entry_set_text (impl->w_device,   "");
  gtk_entry_set_text (impl->w_apn, "");
  gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_baudrate), 3);
  gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_flowcontrol), 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_header_comp), FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_data_comp), FALSE);
  gtk_expander_set_expanded (impl->w_comp_info_expander, FALSE);
}

static const char *
impl_get_display_name (NetworkManagerDialupUI *self)
{
  return _("GPRS over mobile via Bluetooth");
}

static const char *
impl_get_service_name (NetworkManagerDialupUI *self)
{
  return "org.freedesktop.NetworkManager.ppp";
}

static const char *
impl_get_service_type (NetworkManagerDialupUI *self)
{
  return "btgprs";
}

static GtkWidget *
impl_get_widget (NetworkManagerDialupUI *self, GSList *properties, const char *connection_name)
{
  GSList *i;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;
  gboolean should_expand = FALSE;

  btgprs_clear_widget (impl);

  if (connection_name != NULL)
    gtk_entry_set_text (impl->w_connection_name, connection_name);

  for (i = properties; i != NULL && g_slist_next (i) != NULL; i = g_slist_next (g_slist_next (i))) {
    const char *key;
    const char *value;

    key = i->data;
    value = (g_slist_next (i))->data;

    if (strcmp (key, "device") == 0) {
      gtk_entry_set_text (impl->w_device, value);

    } else if (strcmp (key, "channel") == 0) {
      gtk_entry_set_text (impl->w_channel, value);

    } else if (strcmp (key, "apn") == 0) {
      gtk_entry_set_text (impl->w_apn, value);

    } else if (strcmp (key, "baudrate") == 0) {
      gint baudrate_cbox_sel = baudrate_stoi(value);

      if ( baudrate_cbox_sel >= 0 ) {
	gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_baudrate), baudrate_cbox_sel);
      } else {
	// how to set custom text?
      }

    } else if (strcmp (key, "flowcontrol") == 0) {
      gint flowcontrol_cbox_sel = flowcontrol_stoi(value);

      gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_flowcontrol), flowcontrol_cbox_sel);

    } else if ( (strcmp (key, "comp-gprs-header") == 0) &&
		(strcmp (value, "yes") == 0) ) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_header_comp), TRUE);
      should_expand = TRUE;

    } else if ( (strcmp (key, "comp-gprs-data") == 0) &&
		(strcmp (value, "yes") == 0) ) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_data_comp), TRUE);
      should_expand = TRUE;

    }
  }

  gtk_expander_set_expanded (impl->w_comp_info_expander, should_expand);
  gtk_container_resize_children (GTK_CONTAINER (impl->widget));

  return impl->widget;
}

static GSList *
impl_get_properties (NetworkManagerDialupUI *self)
{
  GSList *data;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;
  const char *connection_name;
  const char *channel;
  const char *apn;
  const char *device;
  gint        baudrate_index;
  gboolean    use_gprs_header_comp;
  gboolean    use_gprs_data_comp;

  connection_name  = gtk_entry_get_text (impl->w_connection_name);
  device           = gtk_entry_get_text (impl->w_device);
  channel          = gtk_entry_get_text (impl->w_channel);
  apn              = gtk_entry_get_text (impl->w_apn);
  baudrate_index   = gtk_combo_box_get_active (GTK_COMBO_BOX (impl->w_baudrate));

  use_gprs_header_comp  = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_header_comp));
  use_gprs_data_comp    = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_data_comp));

  data = NULL;

  data = g_slist_append (data, g_strdup ("wvdial"));
  data = g_slist_append (data, g_strdup ("yes"));

  data = g_slist_append (data, g_strdup ("btdevice"));
  data = g_slist_append (data, g_strdup (device));

  data = g_slist_append (data, g_strdup ("btchannel"));
  data = g_slist_append (data, g_strdup (channel));

  data = g_slist_append (data, g_strdup ("apn"));
  data = g_slist_append (data, g_strdup (apn));

  data = g_slist_append (data, g_strdup ("baudrate"));
  if ( baudrate_index == -1 ) {
    data = g_slist_append (data, g_strdup (gtk_combo_box_get_active_text (GTK_COMBO_BOX (impl->w_baudrate)) ));
  } else {
    data = g_slist_append (data, g_strdup (baudrate_itos (baudrate_index)));
  }

  data = g_slist_append (data, g_strdup ("flowcontrol"));
  data = g_slist_append (data, g_strdup (flowcontrol_itos(gtk_combo_box_get_active (GTK_COMBO_BOX (impl->w_flowcontrol)))) );

  data = g_slist_append (data, g_strdup ("comp-gprs-header"));
  data = g_slist_append (data, use_gprs_header_comp ? g_strdup ("yes") : g_strdup("no"));

  data = g_slist_append (data, g_strdup ("comp-gprs-data"));
  data = g_slist_append (data, use_gprs_data_comp ? g_strdup ("yes") : g_strdup("no"));

  return data;
}

static char *
impl_get_connection_name (NetworkManagerDialupUI *self)
{
  const char *name;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;

  name = gtk_entry_get_text (impl->w_connection_name);
  if (name != NULL)
    return g_strdup (name);
  else
    return NULL;
}


static gboolean
impl_is_valid (NetworkManagerDialupUI *self)
{
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;
  gboolean is_valid;
  const char *connection_name;
  const char *device;
  const char *channel;
  const char *apn;

  connection_name        = gtk_entry_get_text (impl->w_connection_name);
  device                 = gtk_entry_get_text (impl->w_device);
  channel                = gtk_entry_get_text (impl->w_channel);
  apn                    = gtk_entry_get_text (impl->w_apn);

  is_valid = TRUE;

  if ( (strlen (connection_name) == 0) ||
       (strlen (device) == 0) ||
       (strstr (device, " ") != NULL)  ||
       (strstr (device, "\t") != NULL) ||
       (strlen (apn) == 0) ||
       (strstr (apn, " ") != NULL)  ||
       (strstr (apn, "\t") != NULL) ||
       (strlen (channel) == 0) ||
       (strstr (channel, " ") != NULL)  ||
       (strstr (channel, "\t") != NULL) ) {

    is_valid = FALSE;
  }

  return is_valid;
}

static void
editable_changed (GtkEditable *editable, gpointer user_data)
{
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) user_data;

  if (impl->callback != NULL) {
    gboolean is_valid;

    is_valid = impl_is_valid (&(impl->parent));
    impl->callback (&(impl->parent), is_valid, impl->callback_user_data);
  }
}


static void
impl_set_validity_changed_callback (NetworkManagerDialupUI *self,
				    NetworkManagerDialupUIDialogValidityCallback callback,
				    gpointer user_data)
{
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;

  impl->callback = callback;
  impl->callback_user_data = user_data;
}

static void
impl_get_confirmation_details (NetworkManagerDialupUI *self, gchar **retval)
{
  GString *buf;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;
  const char *connection_name;
  const char *device;
  const char *channel;
  const char *apn;
  gint        baudrate_index;
  const char *baudrate;
  gint        flowcontrol_index;
  gboolean    use_gprs_header_comp;
  gboolean    use_gprs_data_comp;

  connection_name        = gtk_entry_get_text (impl->w_connection_name);
  device                 = gtk_entry_get_text (impl->w_device);
  channel                = gtk_entry_get_text (impl->w_channel);
  apn                    = gtk_entry_get_text (impl->w_apn);
  baudrate_index         = gtk_combo_box_get_active( GTK_COMBO_BOX (impl->w_baudrate) );
  flowcontrol_index      = gtk_combo_box_get_active( GTK_COMBO_BOX (impl->w_flowcontrol) );
  use_gprs_header_comp   = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_header_comp));
  use_gprs_data_comp     = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_data_comp));

  if ( baudrate_index == -1 ) {
    baudrate = gtk_combo_box_get_active_text( GTK_COMBO_BOX (impl->w_baudrate) );
  } else {
    baudrate = baudrate_itos( baudrate_index );
  }


  // This is risky, should be variable length depending on actual data!
  buf = g_string_sized_new (1024);

  g_string_append (buf, _("The following GPRS via Bluetooth connection will be created:"));
  g_string_append (buf, "\n\n\t");
  g_string_append_printf (buf, _("Name:  %s"), connection_name);
  g_string_append (buf, "\n\n\t");

  g_string_append_printf (buf, _("BT Device:  %s"), device);
  g_string_append (buf, "\n\t");
  g_string_append_printf (buf, _("BT Channel:  %s"), channel);
  g_string_append (buf, "\n\t");
  g_string_append_printf (buf, _("Access Point Name:  %s"), apn);
  g_string_append (buf, "\n\t");
  g_string_append_printf (buf, _("Baud Rate:  %s"), baudrate);
  g_string_append (buf, "\n\t");
  g_string_append_printf (buf, _("Flow Control:  %s"), flowcontrol_itos (flowcontrol_index));
  g_string_append (buf, "\n\t");

  if ( use_gprs_header_comp ||
       use_gprs_data_comp ) {

    g_string_append_printf( buf, _("Use Header Compression: %s"), ((use_gprs_header_comp) ? _("Yes") : _("No")));
    g_string_append (buf, "\n\t");
    g_string_append_printf( buf, _("Use Data Compression: %s"), ((use_gprs_data_comp) ? _("Yes") : _("No")));
    g_string_append (buf, "\n\t");
  }

  *retval = g_string_free (buf, FALSE);
}

static gboolean
import_from_file (NetworkManagerDialupUIImpl *impl, const char *path)
{
  char *basename;
  GKeyFile *keyfile;
  gboolean file_is_good;

  file_is_good = FALSE;
  basename = g_path_get_basename (path);

  keyfile = g_key_file_new ();
  if (g_key_file_load_from_file (keyfile, path, 0, NULL)) {
    char *connection_name = NULL;
    char *device = NULL;
    char *channel = NULL;
    char *apn = NULL;
    char *baudrate = NULL;
    char *flowcontrol = NULL;
    char *comp_gprs_header = NULL;
    char *comp_gprs_data = NULL;
    gboolean should_expand;
    int   baudrate_index = -1;

    connection_name  = g_key_file_get_string (keyfile, "btgprs", "description", NULL);
    device           = g_key_file_get_string (keyfile, "btgprs", "device", NULL);
    channel          = g_key_file_get_string (keyfile, "btgprs", "channel", NULL);
    apn              = g_key_file_get_string (keyfile, "btgprs", "apn", NULL);
    baudrate         = g_key_file_get_string (keyfile, "btgprs", "baudrate", NULL);
    flowcontrol      = g_key_file_get_string (keyfile, "btgprs", "flowcontrol", NULL);
    comp_gprs_header = g_key_file_get_string (keyfile, "btgprs", "comp-gprs-header", NULL);
    comp_gprs_data   = g_key_file_get_string (keyfile, "btgprs", "comp-gprs-data", NULL);

    file_is_good = FALSE;

    /* sanity check data */
    if ( (connection_name != NULL) &&
	 (device != NULL ) &&
	 (channel != NULL) &&
	 (apn != NULL) &&
	 (baudrate != NULL) &&
	 (strlen (connection_name) > 0) &&
	 (strlen (device) > 0) &&
	 (strlen (channel) > 0) &&
	 (strlen (apn) > 0) &&
	 (strlen (baudrate) > 0) &&
	 (strlen (connection_name) > 0) ) {

      // basics ok
      file_is_good = TRUE;
    }

    if (flowcontrol != NULL) {
      if ( (strlen (flowcontrol) == 0) ||
	   ( (strcmp("none", flowcontrol) != 0) &&
	     (strcmp("xonxoff", flowcontrol) != 0) &&
	     (strcmp("crtscts", flowcontrol) != 0) )
	   ) {
	file_is_good = FALSE;
      }
    }

    if (baudrate != NULL) {
      if ( (strlen (baudrate) == 0) ) {
	baudrate_index = baudrate_stoi(baudrate);
	file_is_good = FALSE;
      }
    }

    if (file_is_good) {
      should_expand = FALSE;

      gtk_entry_set_text (impl->w_connection_name, connection_name);
      gtk_entry_set_text (impl->w_device, device);
      gtk_entry_set_text (impl->w_channel, channel);
      gtk_entry_set_text (impl->w_apn, apn);

      if ( baudrate != NULL ) {
	if (baudrate_index == -1 ) {
	  // custom, set text
	} else {
	  gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_baudrate), baudrate_index);
	}
      }

      if (flowcontrol != NULL) {
	gtk_combo_box_set_active (GTK_COMBO_BOX (impl->w_flowcontrol),
				  flowcontrol_stoi (flowcontrol) );
      }

      if ( (comp_gprs_header != NULL) && (strcmp(comp_gprs_header, "yes") == 0) ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_header_comp), TRUE);
	should_expand = TRUE;
      }

      if ( (comp_gprs_data != NULL) && (strcmp(comp_gprs_data, "yes") == 0) ) {
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (impl->w_use_gprs_data_comp), TRUE);
	should_expand = TRUE;
      }

      gtk_expander_set_expanded (impl->w_comp_info_expander, should_expand);
    } else {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_CLOSE,
				       _("Cannot import settings"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("The GPRS via Bluetooth dialup settings file '%s' does not contain valid data."), basename);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }

    g_key_file_free (keyfile);

    g_free (connection_name);
    g_free (device);
    g_free (channel);
    g_free (apn);
    g_free (baudrate);
    g_free (flowcontrol);
    g_free (comp_gprs_header);
    g_free (comp_gprs_data);
  }

  g_free (basename);

  return file_is_good;
}


static void
import_button_clicked (GtkButton *button, gpointer user_data)
{
  char *filename = NULL;
  GtkWidget *dialog;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) user_data;

  dialog = gtk_file_chooser_dialog_new (_("Select file to import"),
					NULL,
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    /*printf ("User selected '%s'\n", filename);*/

  }

  gtk_widget_destroy (dialog);

  if (filename != NULL) {
    import_from_file (impl, filename);
    g_free (filename);
  }
}


static gboolean
impl_can_export (NetworkManagerDialupUI *self)
{
  return TRUE;
}

static gboolean
impl_import_file (NetworkManagerDialupUI *self, const char *path)
{
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;

  return import_from_file (impl, path);
}

static gboolean
export_to_file (NetworkManagerDialupUIImpl *impl, const char *path,
		GSList *properties, const char *connection_name)
{
  FILE *f;
  GSList *i;
  const char *device = NULL;
  const char *channel = NULL;
  const char *apn = NULL;
  const char *baudrate = NULL;
  const char *flowcontrol = NULL;
  const char *comp_gprs_header = NULL;
  const char *comp_gprs_data = NULL;
  gboolean ret;

  /*printf ("in export_to_file; path='%s'\n", path);*/

  for (i = properties; i != NULL && g_slist_next (i) != NULL; i = g_slist_next (g_slist_next (i))) {
    const char *k;
    const char *value;

    k = i->data;
    value = (g_slist_next (i))->data;

    if (strcmp (k, "device") == 0) {
      device = value;
    } else if (strcmp (k, "channel") == 0) {
      channel = value;
    } else if (strcmp (k, "apn") == 0) {
      apn = value;
    } else if (strcmp (k, "baudrate") == 0) {
      baudrate = value;
    } else if (strcmp (k, "flowcontrol") == 0) {
      flowcontrol = value;
    } else if (strcmp (k, "comp-gprs-header") == 0) {
      comp_gprs_header = value;
    } else if (strcmp (k, "comp-gprs-data") == 0) {
      comp_gprs_data = value;
    }
  }


  f = fopen (path, "w");
  if (f != NULL) {

    fprintf (f,
	     "[btgprs]\n"
	     "description=%s\n"
	     "device=%s\n"
	     "channel=%s\n"
	     "apn=%s\n"
	     "baudrate=%s\n"
	     "flowcontrol=%s\n"
	     "comp-gprs-header=%s\n"
	     "comp-gprs-data=%s\n",
	     /* Description */ connection_name,
	     /* Device */      device,
	     /* Channel */     channel,
	     /* APN */         apn,
	     /* Baud Rate */   baudrate,
	     /* Flow Ctrl */   flowcontrol,
	     /* Hdr Comp */    comp_gprs_header,
	     /* Data Comp */   comp_gprs_data
	     );

    fclose (f);
    ret = TRUE;
  }
  else
    ret = FALSE;
  return ret;
}


static gboolean
impl_export (NetworkManagerDialupUI *self, GSList *properties, const char *connection_name)
{
  char *suggested_name;
  char *path = NULL;
  GtkWidget *dialog;
  NetworkManagerDialupUIImpl *impl = (NetworkManagerDialupUIImpl *) self->data;

  /*printf ("in impl_export\n");*/

  dialog = gtk_file_chooser_dialog_new (_("Save as..."),
					NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);

  suggested_name = g_strdup_printf ("%s.nmd", connection_name);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), suggested_name);
  g_free (suggested_name);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {

    path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
/*     printf ("User selected '%s'\n", path); */

  }

  gtk_widget_destroy (dialog);

  if (path != NULL) {
    if (g_file_test (path, G_FILE_TEST_EXISTS)) {
      int response;

      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_CANCEL,
				       _("A file named \"%s\" already exists."), path);
      gtk_dialog_add_buttons (GTK_DIALOG (dialog), "_Replace", GTK_RESPONSE_OK, NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Do you want to replace it with the one you are saving?"));
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      if (response != GTK_RESPONSE_OK)
	goto out;
    }

    if (!export_to_file (impl, path, properties, connection_name)) {

      dialog = gtk_message_dialog_new (NULL,
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_CLOSE,
				       _("Failed to export configuration"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("Failed to save file %s"), path);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }
  }

out:
  g_free (path);

  return TRUE;
}


static NetworkManagerDialupUI*
impl_get_object (void)
{
  char *glade_file;
  NetworkManagerDialupUIImpl *impl;

  impl = g_new0 (NetworkManagerDialupUIImpl, 1);

  impl->last_fc_dir = NULL;

  glade_file = g_strdup_printf ("%s/%s", GLADEDIR, "nm-btgprs-dialog.glade");
  impl->xml = glade_xml_new (glade_file, NULL, GETTEXT_PACKAGE);
  g_free( glade_file );
  if (impl->xml != NULL) {

    impl->widget = glade_xml_get_widget(impl->xml, "nm-btgprs-widget");

    impl->w_connection_name        = GTK_ENTRY (glade_xml_get_widget (impl->xml, "btgprs-connection-name"));
    impl->w_device                 = GTK_ENTRY (glade_xml_get_widget (impl->xml, "btgprs-device"));
    impl->w_channel                = GTK_ENTRY (glade_xml_get_widget (impl->xml, "btgprs-channel"));
    impl->w_apn                    = GTK_ENTRY (glade_xml_get_widget (impl->xml, "btgprs-apn"));

    impl->w_baudrate               = GTK_COMBO_BOX (glade_xml_get_widget (impl->xml, "btgprs-baudrate"));
    impl->w_flowcontrol            = GTK_COMBO_BOX (glade_xml_get_widget (impl->xml, "btgprs-flowcontrol"));


    impl->w_comp_info_expander     = GTK_EXPANDER (glade_xml_get_widget (impl->xml, "btgprs-comp-information-expander"));

    impl->w_use_gprs_header_comp   = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "btgprs-use-header-comp"));
    impl->w_use_gprs_data_comp     = GTK_CHECK_BUTTON (glade_xml_get_widget (impl->xml, "btgprs-use-data-comp"));
    impl->w_import_button          = GTK_BUTTON (glade_xml_get_widget (impl->xml, "btgprs-import-button"));

    impl->callback                 = NULL;


    gtk_signal_connect (GTK_OBJECT (impl->w_connection_name),
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_device),
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_channel),
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_apn),
			"changed", GTK_SIGNAL_FUNC (editable_changed), impl);
    gtk_signal_connect (GTK_OBJECT (impl->w_import_button),
			"clicked", GTK_SIGNAL_FUNC (import_button_clicked), impl);

    /* make the widget reusable */
    gtk_signal_connect (GTK_OBJECT (impl->widget), "delete-event",
			GTK_SIGNAL_FUNC (gtk_widget_hide_on_delete), NULL);

    btgprs_clear_widget (impl);

    impl->parent.get_display_name              = impl_get_display_name;
    impl->parent.get_service_name              = impl_get_service_name;
    impl->parent.get_service_type              = impl_get_service_type;
    impl->parent.get_widget                    = impl_get_widget;
    impl->parent.get_connection_name           = impl_get_connection_name;
    impl->parent.get_properties                = impl_get_properties;
    impl->parent.set_validity_changed_callback = impl_set_validity_changed_callback;
    impl->parent.is_valid                      = impl_is_valid;
    impl->parent.get_confirmation_details      = impl_get_confirmation_details;
    impl->parent.can_export                    = impl_can_export;
    impl->parent.import_file                   = impl_import_file;
    impl->parent.export                        = impl_export;
    impl->parent.data                          = impl;

    return &(impl->parent);
  } else {
    g_free (impl);
    return NULL;
  }
}

NetworkManagerDialupUI * nm_dialup_properties_factory(void);

NetworkManagerDialupUI *
nm_dialup_properties_factory (void)
{
  return impl_get_object();
}
