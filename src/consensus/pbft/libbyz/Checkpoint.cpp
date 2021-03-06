// Copyright (c) Microsoft Corporation.
// Copyright (c) 1999 Miguel Castro, Barbara Liskov.
// Copyright (c) 2000, 2001 Miguel Castro, Rodrigo Rodrigues, Barbara Liskov.
// Licensed under the MIT license.

#include "Checkpoint.h"

#include "Message_tags.h"
#include "Principal.h"
#include "Replica.h"
#include "parameters.h"
#include "pbft_assert.h"

Checkpoint::Checkpoint(Seqno s, Digest& d, bool stable) :
#ifndef USE_PKEY_CHECKPOINTS
  Message(
    Checkpoint_tag,
    sizeof(Checkpoint_rep) + pbft::GlobalState::get_node().auth_size())
{
#else
  Message(Checkpoint_tag, sizeof(Checkpoint_rep) + pbft_max_signature_size)
{
#endif
  rep().extra = (stable) ? 1 : 0;
  rep().seqno = s;
  rep().digest = d;
  rep().id = pbft::GlobalState::get_node().id();
#ifdef USE_PKEY_CHECKPOINTS
  rep().sig_size = 0;
#endif
  rep().padding = 0;

#ifndef USE_PKEY_CHECKPOINTS
  auth_type = Auth_type::out;
  auth_len = sizeof(Checkpoint_rep);
  auth_src_offset = 0;
#else
  rep().sig_size = pbft::GlobalState::get_node().gen_signature(
    contents(), sizeof(Checkpoint_rep), contents() + sizeof(Checkpoint_rep));
#endif
}

void Checkpoint::re_authenticate(Principal* p, bool stable)
{
#ifndef USE_PKEY_CHECKPOINTS
  if (stable)
    rep().extra = 1;
  auth_type = Auth_type::out;
  auth_len = sizeof(Checkpoint_rep);
  auth_src_offset = 0;
#else
  if (rep().extra != 1 && stable)
  {
    rep().extra = 1;
    rep().sig_size = pbft::GlobalState::get_node().gen_signature(
      contents(), sizeof(Checkpoint_rep), contents() + sizeof(Checkpoint_rep));
  }
#endif
}

bool Checkpoint::pre_verify()
{
  // Checkpoints must be sent by replicas.
  if (!pbft::GlobalState::get_node().is_replica(id()))
  {
    return false;
  }

  // Check signature size.
  if (
    size() - (int)sizeof(Checkpoint_rep) <
    pbft::GlobalState::get_node().auth_size(id()))
  {
    return false;
  }

  return true;
}

bool Checkpoint::convert(Message* m1, Checkpoint*& m2)
{
  if (!m1->has_tag(Checkpoint_tag, sizeof(Checkpoint_rep)))
  {
    return false;
  }
  m1->trim();
  m2 = (Checkpoint*)m1;
  return true;
}
