
create view view_experiment_response as
    select experiment.id as experiment_id, response.*
        from experiment
        join response on experiment.response_id = response.id;

create view view_experiment_error as
    select experiment_id,
       succ_rate,
        avg_angle_err,
        avg_trans_err,
        avg_succ_angle_err,
        avg_succ_trans_err
    from view_experiment_response;

create view view_experiment_detect_roc as
    select experiment_id,
        detect_tp,
        detect_fp,
        detect_fn,
        detect_tn,
        1.0 * detect_tp / (detect_tp + detect_fn) as detect_tp_rate,
        1.0 * detect_fp / (detect_fp + detect_tn) as detect_fp_rate
    from view_experiment_response;
 
create view view_experiment_scores as
    select experiment_id,
        value,
        (1.0 * detect_sipc_acc_score / detect_sipc_objects) as detect_sipc,
        (0.5 * locate_sipc_cscore + 0.25 * locate_sipc_rscore + 0.25 * locate_sipc_tscore) / locate_sipc_frames as locate_sipc
    from view_experiment_response;
 