#include "nm_proc.h"

static struct proc_dir_entry *nm_proc_root;
static struct proc_dir_entry *nm_entries[__NM_PROC_LEN];

static int read_modelinfo(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  if (nm_model.info.valid) 
  {
    len = sprintf(page,"Loaded Model: %s [ID: %u]\n"
                       " - %u hops\n"
                       " - %u endpoints\n",
                       nm_model.info.name,
                       nm_model.info.valid,
                       nm_model.info.n_hops,
                       nm_model.info.n_endpoints);
  } 
  else {
    len = sprintf(page,"No model loaded\n");
  }
  return len;
}

static int write_modelinfo(struct file *filp, const char __user *buf, unsigned long len, void *data)
{

  if ( len > sizeof(nm_model.info))
    return -EOVERFLOW;

  if (copy_from_user(&nm_model.info,buf,sizeof(nm_model.info)))
    return -EINVAL;

  nm_notice(LD_GENERAL,"Loaded model details - Name: '%s' [ID: %u] n_hops: %u n_endpoints: %u\n",
                nm_model.info.name,
                nm_model.info.valid,
                nm_model.info.n_hops,
                nm_model.info.n_endpoints);

  nm_notice(LD_GENERAL,"Initializing data structures\n");

  if (nm_model_initialize() < 0)
    return -EINVAL;

  return len;
}

/*static int write_hoptable(struct file *filp, const char __user *buf, unsigned long len, void *data);*/

int initialize_proc_interface(void)
{
  int ret;
  ret = 0;
  nm_proc_root = proc_mkdir_mode("net-modeler",0644,NULL);

  if (!nm_proc_root){
    ret = -1;
  } else 
  {
    /*CREATE_ENTRY(pathtable,nm_proc_root);*/
    /*CREATE_ENTRY(hoptable,nm_proc_root);*/
    CREATE_ENTRY(nm_entries,modelinfo,nm_proc_root);
  }

  if (ret < 0)
    nm_warn(LD_ERROR,"/proc entry creation failed.");

  return ret;
}

int cleanup_proc_interface(void)
{
  int ret;
  ret= 0;

  /*remove_proc_entry(stringify(pathtable),nm_proc_root);*/
  remove_proc_entry(stringify(modelinfo),nm_proc_root);
  /*remove_proc_entry(stringify(hoptable),nm_proc_root);*/
  remove_proc_entry("net-modeler",NULL);

  return 0;
}
