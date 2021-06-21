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
 * intertask_interface_conf.h
 *
 *  Created on: Oct 21, 2013
 *      Author: winckel
 */

#ifndef FILE_INTERTASK_INTERFACE_CONF_SEEN
#define FILE_INTERTASK_INTERFACE_CONF_SEEN

/*******************************************************************************
 * Intertask Interface Constants
 ******************************************************************************/

#define ITTI_PORT (10007)

/* This is the queue size for signal dumper */
#define ITTI_QUEUE_MAX_ELEMENTS (64 * 1024)
#define ITTI_DUMP_MAX_CON (5) /* Max connections in parallel */

#endif /* FILE_INTERTASK_INTERFACE_CONF_SEEN */
