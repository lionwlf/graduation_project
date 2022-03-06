rm -rf msg.pb*
protoc msg.proto --cpp_out ./
rm -rf ../../src/protobuf/*
mv msg.pb.cc ../../src/protobuf/msg.pb.cc
