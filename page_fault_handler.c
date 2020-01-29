
//Handle the page fault
void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	__page_fault_handler_with_buffering(curenv, fault_va);
}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{

	//cprintf("fault va %x \n",fault_va);
	//TODO: [PROJECT 2019 - MS1 - [3] Page Fault Handler: PLACEMENT & REPLACEMENT CASES]
	// Write your code here, remove the panic and write your code

	//refer to the project documentation for the detailed steps of the page fault handler
	// 1 placement
	uint32 *page_table =NULL;
	uint32 WSsize = env_page_ws_get_size(curenv);
	struct Frame_Info *ptr_frame_info ;
	ptr_frame_info=get_frame_info(curenv->env_page_directory,(void *)fault_va,&page_table);
	uint32 i=curenv->page_last_WS_index;
	//ptr_frame_info->environment = curenv;
	//ptr_frame_info->va = fault_va;
	uint32 victim;
	if( WSsize < curenv->page_WS_max_size)
	{
		//1.1 placement
		uint32 page_permissions = pt_get_page_permissions(curenv, fault_va);
		////1.1.1 placement
		if((page_permissions & PERM_BUFFERED )==PERM_BUFFERED)
		{
			pt_set_page_permissions(curenv, fault_va, PERM_PRESENT, PERM_BUFFERED);
			//pt_set_page_permissions(curenv, fault_va, 0,  );
			ptr_frame_info->isBuffered = 0;
			//1.1.2 placement
			if ((page_permissions & PERM_MODIFIED)==PERM_MODIFIED )
			{
				bufferlist_remove_page(&modified_frame_list, ptr_frame_info);
			}
			else
			{
				bufferlist_remove_page(&free_frame_list, ptr_frame_info);

			}
		}
		//1.2placement
		else
		{
			//1.2.1 placement
			int alloc = allocate_frame(&ptr_frame_info);
			alloc =map_frame(curenv->env_page_directory,  ptr_frame_info,(void *)fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
			//1.2.2 placement
			uint32 ret =   pf_read_env_page( curenv, (uint32 *)fault_va);
			//1.2.3 placement
			if (ret == E_PAGE_NOT_EXIST_IN_PF)
			{

				if((fault_va >= USTACKBOTTOM )&&(  fault_va  < USTACKTOP))
				{
					ret =  pf_add_empty_env_page( curenv, fault_va, 0);
				}
				else
				{
					panic("PAGE NOT FOUND");
				}
			}

		}
		//1.3 placement
		env_page_ws_set_entry( curenv , i,  fault_va);
		if(i == curenv->page_WS_max_size-1 )
		{
			curenv->page_last_WS_index=0;

		}
		else
		{
			curenv->page_last_WS_index=i+1;
		}
	}
	else
	{
		// replacement 3 modified clock alogrithm
		uint32 firstpos = curenv->page_last_WS_index;
		bool flagout = 0 ;
		bool flagin = 0 ;
		while ((flagout == 0)&&(flagin == 0))
		{
			uint32 currentpos = curenv->page_last_WS_index;
			uint32 vaddress1 = env_page_ws_get_virtual_address( curenv , currentpos);
			uint32 page_permissions = pt_get_page_permissions( curenv, vaddress1);
			if(((page_permissions & PERM_USED )== 0 ) && ((page_permissions  &PERM_MODIFIED)== 0 ) )
			{
				victim = vaddress1 ;
				i = curenv->page_last_WS_index;
				//cprintf("outer loop victim @ index %d\n", i);
						env_page_ws_clear_entry(curenv,i);
				curenv->page_last_WS_index++;
				if(curenv->page_last_WS_index == curenv->page_WS_max_size)
				{
					curenv->page_last_WS_index=0;
				}
				flagout = 1;
				break;
			}
			curenv->page_last_WS_index++;
			if(curenv->page_last_WS_index == curenv->page_WS_max_size)
			{
				curenv->page_last_WS_index=0;
			}
			if(curenv->page_last_WS_index == firstpos && flagout == 0 )
			{
				while(flagin ==0){
					uint32 cupos = curenv->page_last_WS_index;
					uint32 vaddress2 = env_page_ws_get_virtual_address( curenv , cupos);
					uint32 page_permissions = pt_get_page_permissions( curenv, vaddress2);
					if((page_permissions & PERM_USED) == 0)
					{
						victim = vaddress2;
						i = curenv->page_last_WS_index;
						//cprintf("inner loop victim @ index %d\n", i);
						env_page_ws_clear_entry(curenv,i);
						curenv->page_last_WS_index++;
						if(curenv->page_last_WS_index == curenv->page_WS_max_size)
						{
							curenv->page_last_WS_index=0;
						}
						flagin = 1;
						break;
					}
					pt_set_page_permissions(curenv, vaddress2, 0, PERM_USED );
				   //cprintf("clear used bit of %x\n",vaddress2);

					curenv->page_last_WS_index++;
					if(curenv->page_last_WS_index == curenv->page_WS_max_size)
					{
						curenv->page_last_WS_index=0;
					}
					if((curenv->page_last_WS_index==firstpos) && (flagin==0) )
					{
						//cprintf("break====\n");
						break;
					}
				}
			}

		}
		//cprintf("####################victim index = %d\n",i);
		//4 replacement
		uint32 *page_table =NULL;
		struct Frame_Info *ptr_frame_infonew ;
		ptr_frame_infonew =get_frame_info(curenv->env_page_directory,(void *)victim,&page_table);

		//int j = get_page_table(curenv->env_page_directory,victim , int create, uint32 **ptr_page_table);

		uint32 sizebufferlist = getModifiedBufferLength();
		ptr_frame_infonew->isBuffered=1;
		//ptr_frame_infonew->environment= curenv;
		//ptr_frame_infonew->va = victim;
		pt_set_page_permissions(curenv, victim, PERM_BUFFERED, PERM_PRESENT);
		//pt_set_page_permissions(curenv,victim,  PERM_BUFFERED  ,0);
		// 5 replacement
		uint32 page_permissions = pt_get_page_permissions(curenv, victim);
		if((page_permissions & PERM_MODIFIED) ==  PERM_MODIFIED )
		{
			//cprintf("modified\n");
			bufferList_add_page(& modified_frame_list,  ptr_frame_infonew);
			uint32 size = LIST_SIZE(&modified_frame_list);
			if(sizebufferlist== size)
			{
				LIST_FOREACH(ptr_frame_infonew, &modified_frame_list)
				{
					uint32 address = ptr_frame_infonew ->va;
					int ret1 =  pf_update_env_page(curenv ,(uint32*)address,ptr_frame_infonew);
					pt_set_page_permissions(curenv, address, 0, PERM_MODIFIED );

					bufferlist_remove_page(&modified_frame_list, ptr_frame_infonew);
					bufferList_add_page(& free_frame_list,  ptr_frame_infonew);

					//free_frame(ptr_frame_infonew);
				}

			}
		}
		else
		{
			//cprintf("not modified \n");
			bufferList_add_page(& free_frame_list,  ptr_frame_infonew);
		}
		// last placement
		//1.1 placement
		//	cprintf("placement after victim\n");
		page_permissions = pt_get_page_permissions(curenv, fault_va);
		////1.1.1 placement
		if((page_permissions & PERM_BUFFERED )==PERM_BUFFERED)
		{

			//cprintf("victim buffered\n");
			pt_set_page_permissions(curenv, fault_va, PERM_PRESENT, PERM_BUFFERED);
			//	pt_set_page_permissions(curenv, fault_va, 0,  );
			ptr_frame_info->isBuffered = 0;
			//1.1.2 placement
			if ((page_permissions & PERM_MODIFIED)==PERM_MODIFIED )
			{
				bufferlist_remove_page(&modified_frame_list, ptr_frame_info);
			}
			else
			{
				bufferlist_remove_page(&free_frame_list, ptr_frame_info);

			}
		}
		//1.2placement
		else
		{
			//cprintf("inside else not Buffered\n");
			//1.2.1 placement
			int alloc = allocate_frame(&ptr_frame_info);
			alloc =map_frame(curenv->env_page_directory,  ptr_frame_info,(void *)fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
			//1.2.2 placement
			uint32 ret =   pf_read_env_page( curenv, (uint32 *)fault_va);
			//1.2.3 placement
			if (ret == E_PAGE_NOT_EXIST_IN_PF)
			{

				if((fault_va >= USTACKBOTTOM )&&(  fault_va  < USTACKTOP))
				{
					ret =  pf_add_empty_env_page( curenv, fault_va, 0);
				}
				else
				{
					panic("PAGE NOT FOUND");
				}
			}

		}
		//1.3 placement
		//cprintf("updating env %d\n",i);

		env_page_ws_set_entry( curenv , i, fault_va);
		if(i == curenv->page_WS_max_size-1 )
		{
			curenv->page_last_WS_index=0;

		}
		else
		{
			curenv->page_last_WS_index=i+1;
		}


	}
	//cprintf("fault handler end\n");
}

