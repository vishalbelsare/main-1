--
-- Copyright (c) 2014 OpenM++
-- This code is licensed under MIT license (see LICENSE.txt for details)
--

-- BEGIN TRANSACTION;

--
-- list of ids, must be positive.
-- id < 100 reserved for development and testing
-- also for types id < 100 reserved for bult-in types
--
INSERT INTO id_lst (id_key, id_value) VALUES ('openmpp',       100);
INSERT INTO id_lst (id_key, id_value) VALUES ('lang_id',       100);
INSERT INTO id_lst (id_key, id_value) VALUES ('model_id',      100);
INSERT INTO id_lst (id_key, id_value) VALUES ('type_hid',      100);
INSERT INTO id_lst (id_key, id_value) VALUES ('parameter_hid', 100);
INSERT INTO id_lst (id_key, id_value) VALUES ('table_hid',     100);
INSERT INTO id_lst (id_key, id_value) VALUES ('run_id_set_id', 100);
INSERT INTO id_lst (id_key, id_value) VALUES ('task_id',       100);
INSERT INTO id_lst (id_key, id_value) VALUES ('task_run_id',   100);

--
-- Languages and word list
--
INSERT INTO lang_lst (lang_id, lang_code, lang_name) VALUES (0, 'EN', 'English');
INSERT INTO lang_lst (lang_id, lang_code, lang_name) VALUES (1, 'FR', 'Français');

-- built-in words
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'all', 'All');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'min', 'Min');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'max', 'Max');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (1, 'all', 'Toutes');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (1, 'min', 'Min');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (1, 'max', 'Max');

-- model runtime messages from library
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Model', 'Model');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'FAILED', 'FAILED');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Done.', 'Done.');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Read', 'Read');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Writing Output Tables Expressions', 'Writing Output Tables Expressions');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Run: %d', 'Run: %d');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Subsample %d', 'Subsample %d');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Restart from subsample %d', 'Restart from subsample %d');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'MPI error: send %d unfinished from %d to %d', 'MPI error: send %d unfinished from %d to %d');

-- model runtime messages from use .mpp .ompp
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Case seed : %.0f\t-\tCase sample: %d\t-\tCheckSum : %.6f', 'Case seed : %.0f\t-\tCase sample: %d\t-\tCheckSum : %.6f');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Member=%d create start population', 'Member=%d create start population');
INSERT INTO lang_word (lang_id, word_code, word_value) VALUES (0, 'Member=%d simulation progress=%d%%', 'Member=%d simulation progress=%d%%');
  
--
-- built-in types: type name used as unique type digest
--
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (1, 'char',  '_char_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (2, 'schar', '_schar_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (3, 'short', '_short_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (4, 'int',   '_int_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (5, 'long',  '_long_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (6, 'llong', '_llong_', 0, 1);
-- INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (7, 'bool',  '_bool_', 1, 2);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (8, 'uchar',    '_uchar_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (9, 'ushort',   '_ushort_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (10, 'uint',     '_uint_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (11, 'ulong',   '_ulong_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (12, 'ullong',  '_ullong_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (13, 'float',   '_float_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (14, 'double',  '_double_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (15, 'ldouble', '_ldouble_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (16, 'Time',    '_Time_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (17, 'real',    '_real_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (18, 'integer', '_integer_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (19, 'counter', '_counter_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (20, 'big_counter', '_big_counter_', 0, 1);
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (21, 'file',    '_file_', 0, 1);

-- 
-- built-in types: logical type
--
INSERT INTO type_dic (type_hid, type_name, type_digest, dic_id, total_enum_id) VALUES (7, 'bool', '_bool_', 1, 2);

INSERT INTO type_dic_txt (type_hid, lang_id, descr, note) VALUES (7, 0, 'logical type', NULL);
INSERT INTO type_dic_txt (type_hid, lang_id, descr, note) VALUES (7, 1, 'logical type [no label (FR)]', NULL);

INSERT INTO type_enum_lst (type_hid, enum_id, enum_name) VALUES (7, 0, 'false');
INSERT INTO type_enum_lst (type_hid, enum_id, enum_name) VALUES (7, 1, 'true');

INSERT INTO type_enum_txt (type_hid, enum_id, lang_id, descr, note) VALUES (7, 0, 0, 'False', NULL);
INSERT INTO type_enum_txt (type_hid, enum_id, lang_id, descr, note) VALUES (7, 0, 1, 'Faux', NULL);
INSERT INTO type_enum_txt (type_hid, enum_id, lang_id, descr, note) VALUES (7, 1, 0, 'True', NULL);
INSERT INTO type_enum_txt (type_hid, enum_id, lang_id, descr, note) VALUES (7, 1, 1, 'Vrai', NULL);

-- COMMIT;