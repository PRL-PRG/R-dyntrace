create index ix_arguments on arguments (id);
create index ix_calls on calls (id);
create index ix_functions on functions (id);
create index ix_promises on promises (id);

create index ix1_promise_associations on promise_associations (promise_id);
create index ix2_promise_associations on promise_associations (call_id);
create index ix3_promise_associations on promise_associations (argument_id);

create index ix1_promise_evaluations on promise_evaluations (promise_id);
create index ix2_promise_evaluations on promise_evaluations (from_call_id);

