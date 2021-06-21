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

/*! \file gtpv1_u_messages_def.h
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/

MESSAGE_DEF(GTPV1U_CREATE_TUNNEL_REQ, MESSAGE_PRIORITY_MED,
            Gtpv1uCreateTunnelReq, gtpv1uCreateTunnelReq)
MESSAGE_DEF(GTPV1U_CREATE_TUNNEL_RESP, MESSAGE_PRIORITY_MED,
            Gtpv1uCreateTunnelResp, gtpv1uCreateTunnelResp)
MESSAGE_DEF(GTPV1U_UPDATE_TUNNEL_REQ, MESSAGE_PRIORITY_MED,
            Gtpv1uUpdateTunnelReq, gtpv1uUpdateTunnelReq)
MESSAGE_DEF(GTPV1U_UPDATE_TUNNEL_RESP, MESSAGE_PRIORITY_MED,
            Gtpv1uUpdateTunnelResp, gtpv1uUpdateTunnelResp)
MESSAGE_DEF(GTPV1U_DELETE_TUNNEL_REQ, MESSAGE_PRIORITY_MED,
            Gtpv1uDeleteTunnelReq, gtpv1uDeleteTunnelReq)
MESSAGE_DEF(GTPV1U_DELETE_TUNNEL_RESP, MESSAGE_PRIORITY_MED,
            Gtpv1uDeleteTunnelResp, gtpv1uDeleteTunnelResp)
MESSAGE_DEF(GTPV1U_TUNNEL_DATA_IND, MESSAGE_PRIORITY_MED, Gtpv1uTunnelDataInd,
            gtpv1uTunnelDataInd)
MESSAGE_DEF(GTPV1U_TUNNEL_DATA_REQ, MESSAGE_PRIORITY_MED, Gtpv1uTunnelDataReq,
            gtpv1uTunnelDataReq)
