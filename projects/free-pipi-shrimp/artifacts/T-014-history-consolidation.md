# T-014：双仓库历史整理记录

- 项目：free-pipi-shrimp；负责人：project-manager；日期：2026-10-13。
- 授权来源：用户在本会话要求整理 v0.0.4 之上的全部项目提交，按功能归并、删除 merge commit，并一并处理成果物仓库。
- OPC 基线：`464ca52aafc6c73ff780a7120acd3bf63c9d31dd`（v0.0.4）；改写前项目头：`53e4c1de6601c087980670ecec0c257d17acae01`；T-012 头：`4920d5891d3148c105efcdc9d2442496a2699015`。
- 成果物改写前 main：`607c571ce84c5f125173906949656fb29beba4d8`；T-012 头：`a4a94144742dce0cf604aeecb03b1e020765053b`；保留原初始化提交 `4d023fdcb6190d710512e98d7c03c790c715b234`。
- 恢复点：OPC `archive/free-pipi-shrimp-main-pre-linear-20261012-53e4c1d`、`archive/free-pipi-shrimp-t012-pre-linear-20261012-4920d58`；成果物 `archive/main-pre-linear-20261012-607c571`、`archive/t012-pre-linear-20261012-a4a9414`。四个标签在改写前已推送并保留，既有归档和框架发布标签不移动。
- 范围：OPC 项目主线及既有 T-012 分支、成果物 main 及既有 T-012 分支；不把 T-012 合入主线，不变更业务状态、负责人、成果内容、批准或评审结论。
- OPC 22 个提交（含 4 个 merge）归并为 6 个功能提交加本次版本追踪提交；成果物 main 10 个提交（含 2 个 merge）归并为初始化、T-003 视觉基线、T-013 文案来源、T-005 封面四个提交。
- 功能归并表表示补丁归属；组内中间阶段仍由恢复标签追溯，不宣称每个旧中间提交与新功能提交的树都相同。
- 成果物新 main：`5404a33758b83fd5aff00ced55a4d893bf700c08`；新 T-003 功能版本：`b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd`；新 T-012 draft：`cf96f969a4e0ba6fbb95c3d518e0c82fee471047`。
- 既有正文中的作者、生成、评审和批准 SHA 保留为历史来源证据；当前定位以 WORKSPACE、最新索引与本记录为准。
- 验证：成果物 main 与 T-012 新旧 Git 树分别完全相同，覆盖原始文件、路径、Git blob 与 LFS 对象标识；OPC 第 6 个功能提交与旧主线树相同，后续仅修改项目治理追踪，公共框架树保持 v0.0.4。框架校验、测试和远端最终核对由本任务的实际执行回执确认，不将 Git 树核对称为视觉评审或素材实体下载检查。
- T-012 的 3 个 OPC 提交归并为原分支上的 1 个提交；新父提交是整理后的项目主线，保留 A-006 draft 与 T-012 in_progress，不在正文写入本提交自身哈希。

## OPC 项目提交归并

| 原提交 | 功能组 | 新功能提交 |
|---|---|---|
| `34e82fb91bfac4eb6bdc2b7607876d9b9a548f74` | feat(project,T-009): consolidate project baseline and content production team | `f683b6b73443e5a4fa9e51f460b526860e3ed69c` |
| `0979e68846056526c3401d2081fc85c596c50cc2` | feat(project,T-009): consolidate project baseline and content production team | `f683b6b73443e5a4fa9e51f460b526860e3ed69c` |
| `06f015ac7520fa2c876ee95944f7c1ac861690bf` | feat(project,T-009): consolidate project baseline and content production team | `f683b6b73443e5a4fa9e51f460b526860e3ed69c` |
| `d9f09c97755af23580b95a0ca31a1c2e1076e030` | feat(project,T-009): consolidate project baseline and content production team | `f683b6b73443e5a4fa9e51f460b526860e3ed69c` |
| `e29d152285df9842790fe13785a5e7ccd8b70fd1` | feat(project,T-009): consolidate project baseline and content production team | `f683b6b73443e5a4fa9e51f460b526860e3ed69c` |
| `2c1b793efb693ac86b99e314a0eb4d33ce927cc5` | chore(workspace): configure independent production asset repository | `5145d0f2e923d4c55c2da6270f861c6c7225dd04` |
| `27d0af8cc685d112ef3affc7800fea0902f13a94` | feat(T-003): register approved character visual baseline and review evidence | `924d4d4e455a3ab2e9d9a45239ff4718a572f816` |
| `e3edb8fd841d8af744e7aed1ac8ed4f1233396e8` | feat(T-003): register approved character visual baseline and review evidence | `924d4d4e455a3ab2e9d9a45239ff4718a572f816` |
| `edfde4c37a940a844d202624bef7d89c244a3ba6` | feat(T-003): register approved character visual baseline and review evidence | `924d4d4e455a3ab2e9d9a45239ff4718a572f816` |
| `c43fdbb7e390dcd4267f80d78e0ca9e4ae147ce0` | feat(T-003): register approved character visual baseline and review evidence | `924d4d4e455a3ab2e9d9a45239ff4718a572f816` |
| `d567448b65aa4feecbafc9ff270675f1812333a7` | feat(T-003): register approved character visual baseline and review evidence | `924d4d4e455a3ab2e9d9a45239ff4718a572f816` |
| `d7a889ea9b22493e2830652de40f22b658d1010b` | feat(T-004): consolidate Sanxingdui serial plan and project checks | `7bfb7deeb6cedf787fbf1ba2da7cfab93cab50a4` |
| `7dd87caa8833915da5dce6181087088068255b3d` | feat(T-004): consolidate Sanxingdui serial plan and project checks | `7bfb7deeb6cedf787fbf1ba2da7cfab93cab50a4` |
| `9697c6d62f97281f54833a68381b5f94f5445b01` | feat(T-004): consolidate Sanxingdui serial plan and project checks | `7bfb7deeb6cedf787fbf1ba2da7cfab93cab50a4` |
| `0f9896e9d2b124974c83581e5cedf944230021ef` | feat(T-004): consolidate Sanxingdui serial plan and project checks | `7bfb7deeb6cedf787fbf1ba2da7cfab93cab50a4` |
| `ef70b44391686c22bb89fe1102a0c2b083040f64` | feat(T-004): consolidate Sanxingdui serial plan and project checks | `7bfb7deeb6cedf787fbf1ba2da7cfab93cab50a4` |
| `1ff17be0b0c9c9c8ed97c64ec01d210a6cb3335e` | chore(team): configure independent content reviewer | `ad73375bc9f5e07b6f4c0290e9e1279a8d3beaf9` |
| `dba6da1309018633e2b1c48cd3364f3f67866852` | feat(T-005,T-013): consolidate adopted cover, copy provenance and approvals | `3579ad4ef3c68fadfdfbd3f8602d2187e5bddc15` |
| `11ba8195f2b09cb5bfb0631ab5d0738a08f4d8c0` | feat(T-005,T-013): consolidate adopted cover, copy provenance and approvals | `3579ad4ef3c68fadfdfbd3f8602d2187e5bddc15` |
| `e1fd0e808318ce63e099ce6359ae848a57c788c8` | feat(T-005,T-013): consolidate adopted cover, copy provenance and approvals | `3579ad4ef3c68fadfdfbd3f8602d2187e5bddc15` |
| `33e471293e0d941eb00b0ac2b6b7503d6d5b2143` | feat(T-005,T-013): consolidate adopted cover, copy provenance and approvals | `3579ad4ef3c68fadfdfbd3f8602d2187e5bddc15` |
| `53e4c1de6601c087980670ecec0c257d17acae01` | feat(T-005,T-013): consolidate adopted cover, copy provenance and approvals | `3579ad4ef3c68fadfdfbd3f8602d2187e5bddc15` |

## 成果物提交归并

| 原提交 | 功能组 | 新功能提交 |
|---|---|---|
| `4d023fdcb6190d710512e98d7c03c790c715b234` | 初始化 | `4d023fdcb6190d710512e98d7c03c790c715b234` |
| `d8a7b55ead856505f2f1af3cb886ef2e08d24472` | T-003 视觉基线 | `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| `b547bd6ba987381400c61c16baca26532c4a7767` | T-003 视觉基线 | `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| `86661ecc1f287e9bcaea79fab7a9e76de7d5a1a8` | T-003 视觉基线 | `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| `240f418a1a693313cb79e8efddb34cd82b3b9f79` | T-003 视觉基线 | `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| `730429312be85e33b4534b58adad54a207a8b797` | T-003 视觉基线 | `b7e90ac756f996cc35d5e7e9186b52e6f2cdedbd` |
| `a7cca968e1459c6931d6ef7f551cf04f03273b11` | T-013 文案来源 | `9f5cb8e8a50a161583bc72b5f321e4875aed6fc0` |
| `dc4673367017c3b100dc4c3bbe9921ceae3312b5` | T-005 封面模板 | `5404a33758b83fd5aff00ced55a4d893bf700c08` |
| `2ebbb013396ccdace268e3475678410f5699f62e` | T-005 封面模板 | `5404a33758b83fd5aff00ced55a4d893bf700c08` |
| `607c571ce84c5f125173906949656fb29beba4d8` | T-005 封面模板 | `5404a33758b83fd5aff00ced55a4d893bf700c08` |
