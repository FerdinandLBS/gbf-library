#include <ecore.h>

fn_post_message _post_message = 0;
fn_send_message _send_message = 0;
fn_find_module _find_module = 0;
int _my_id = 0;

void ECORE_CALLCONV ecore_onload(int index, fn_find_module find_module, fn_post_message post_message, fn_send_message send_message)
{
    _my_id = index;
    _post_message = post_message;
    _send_message = send_message;
    _find_module = find_module;
}

void ECORE_CALLCONV ecore_onunload(void)
{

}

void ECORE_CALLCONV ecore_onmessage(unsigned message, void* data)
{

}

void ECORE_CALLCONV ecore_onstart(void)
{

}