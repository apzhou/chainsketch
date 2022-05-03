tmp = []
p4_pd.register_hw_sync_bucket_array_1()
p4_pd.register_hw_sync_bucket_array_2()
p4_pd.register_hw_sync_bucket_array_3()
# p4_pd.register_hw_sync_my_reg2()
# for i in range(110):
#     tmp.append(p4_pd.register_read_my_reg2(i , p4_pd.register_flags_t(False))[0])
print("------row 1------")
for i in range(256):
    print(p4_pd.register_read_bucket_array_1(i , p4_pd.register_flags_t(False)))

print("------row 2------")
for i in range(256):
    print(p4_pd.register_read_bucket_array_2(i , p4_pd.register_flags_t(False)))

print("------row 3------")
for i in range(256):
    print(p4_pd.register_read_bucket_array_3(i , p4_pd.register_flags_t(False)))


