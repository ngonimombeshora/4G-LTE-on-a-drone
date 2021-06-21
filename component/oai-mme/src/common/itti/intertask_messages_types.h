/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the terms found in the LICENSE file in the root of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
 * intertask_messages_types.h
 *
 *  Created on: Jan 14, 2014
 *      Author: laurent winckel
 */

#ifndef INTERTASK_MESSAGES_TYPES_H_
#define INTERTASK_MESSAGES_TYPES_H_

typedef struct IttiMsgEmpty_s {
} IttiMsgEmpty;

typedef struct IttiMsgText_s {
  uint32_t size;
  char text[];
} IttiMsgText;

#endif /* INTERTASK_MESSAGES_TYPES_H_ */
