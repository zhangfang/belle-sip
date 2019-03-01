/*
	belle-sip - SIP (RFC3261) library.
	Copyright (C) 2019  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "belle-sip/object++.hh"
#include "belle_sip_internal.h"

namespace bellesip {


void Object::init(){
	static bool offsetDefined = false;
	belle_sip_object_vptr_t *vptr = belle_sip_cpp_object_t_vptr_get();
	if (!offsetDefined){
		vptr->cpp_offset = (int)  ((intptr_t)&mObject - (intptr_t)this);
		offsetDefined = true;
	}
	memset(&mObject, 0, sizeof(mObject)); /*the new allocator does not zero the memory*/
	_belle_sip_object_init(&mObject, vptr);
}

Object::Object(){
	init();
}

Object::Object(const Object &other){
	init();
	mObject.vptr->get_parent()->clone(&mObject, &other.mObject); /*belle_sip_object_t own's clone method*/
}

Object::~Object(){
	if (mObject.ref != -1){
		/*note: throwing an exception here does not work*/
		belle_sip_fatal("bellesip::Object [%p] has been destroyed directly with delete operator. This is prohibited, use unref() instead.", this);
	}
	belle_sip_object_uninit(&mObject);
	belle_sip_message("Object destroyed [%p]", &mObject);
}

Object *Object::ref(){
	belle_sip_object_ref(&mObject);
	return this;
}

void Object::unref(){
	belle_sip_object_unref(&mObject);
}

belle_sip_error_code Object::marshal(char* buff, size_t buff_size, size_t *offset){
	return mObject.vptr->get_parent()->marshal(&mObject, buff, buff_size, offset); /*default to belle_sip_object_t's implementation*/
}

Object *Object::clone()const{
	return new Object(*this);
}

belle_sip_object_t *Object::getCObject(){
	return &mObject;
}

const belle_sip_object_t *Object::getCObject()const{
	return &mObject;
}

Object *Object::getCppObject(void *ptr){
	belle_sip_cpp_object_t *obj = BELLE_SIP_CAST(ptr, belle_sip_cpp_object_t);
	intptr_t cppaddr = (intptr_t)obj - (intptr_t)obj->vptr->cpp_offset;
	return reinterpret_cast<Object*>(cppaddr);
}

const Object *Object::getCppObject(const void *ptr){
	return Object::getCppObject((void*)ptr);
}

}//end of namespace

void belle_sip_cpp_object_delete(belle_sip_object_t *obj){
	bellesip::ObjectCAccessors::doDelete(obj);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(belle_sip_cpp_object_t);
BELLE_SIP_INSTANCIATE_VPTR3(belle_sip_cpp_object_t,belle_sip_object_t,NULL,NULL,bellesip::ObjectCAccessors::sMarshal,NULL,NULL,FALSE,TRUE);
